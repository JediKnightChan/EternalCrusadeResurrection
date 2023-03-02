// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRPawnComponent_CharacterParts.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagAssetInterface.h"
#include "Gameplay/GAS/ECRPlayerOwnedTaggedActor.h"
#include "Gameplay/Player/ECRPlayerState.h"

//////////////////////////////////////////////////////////////////////

FString FECRAppliedCharacterPartEntry::GetDebugString() const
{
	return FString::Printf(
		TEXT("(PartClass: %s, Socket: %s, Instance: %s)"), *GetPathNameSafe(Part.PartClass),
		*Part.SocketName.ToString(), *GetPathNameSafe(SpawnedComponent));
}

//////////////////////////////////////////////////////////////////////

void FECRCharacterPartList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	bool bDestroyedAnyActors = false;
	for (int32 Index : RemovedIndices)
	{
		FECRAppliedCharacterPartEntry& Entry = Entries[Index];
		bDestroyedAnyActors |= DestroyActorForEntry(Entry);
	}

	if (bDestroyedAnyActors)
	{
		OwnerComponent->BroadcastChanged();
	}
}

void FECRCharacterPartList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	bool bCreatedAnyActors = false;
	for (int32 Index : AddedIndices)
	{
		FECRAppliedCharacterPartEntry& Entry = Entries[Index];
		bCreatedAnyActors |= SpawnActorForEntry(Entry);
	}

	if (bCreatedAnyActors)
	{
		OwnerComponent->BroadcastChanged();
	}
}

void FECRCharacterPartList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	bool bChangedAnyActors = false;

	// We don't support dealing with propagating changes, just destroy and recreate
	for (int32 Index : ChangedIndices)
	{
		FECRAppliedCharacterPartEntry& Entry = Entries[Index];

		bChangedAnyActors |= DestroyActorForEntry(Entry);
		bChangedAnyActors |= SpawnActorForEntry(Entry);
	}

	if (bChangedAnyActors)
	{
		OwnerComponent->BroadcastChanged();
	}
}

FECRCharacterPartHandle FECRCharacterPartList::AddEntry(FECRCharacterPart NewPart)
{
	FECRCharacterPartHandle Result;
	Result.PartHandle = PartHandleCounter++;

	if (ensure(OwnerComponent && OwnerComponent->GetOwner() && OwnerComponent->GetOwner()->HasAuthority()))
	{
		FECRAppliedCharacterPartEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.Part = NewPart;
		NewEntry.PartHandle = Result.PartHandle;

		if (SpawnActorForEntry(NewEntry))
		{
			OwnerComponent->BroadcastChanged();
		}

		MarkItemDirty(NewEntry);
	}

	return Result;
}

void FECRCharacterPartList::RemoveEntry(FECRCharacterPartHandle Handle)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FECRAppliedCharacterPartEntry& Entry = *EntryIt;
		if (Entry.PartHandle == Handle.PartHandle)
		{
			const bool bDestroyedActor = DestroyActorForEntry(Entry);
			EntryIt.RemoveCurrent();
			MarkArrayDirty();

			if (bDestroyedActor)
			{
				OwnerComponent->BroadcastChanged();
			}

			break;
		}
	}
}

void FECRCharacterPartList::ClearAllEntries(bool bBroadcastChangeDelegate)
{
	bool bDestroyedAnyActors = false;
	for (FECRAppliedCharacterPartEntry& Entry : Entries)
	{
		bDestroyedAnyActors |= DestroyActorForEntry(Entry);
	}
	Entries.Reset();
	MarkArrayDirty();

	if (bDestroyedAnyActors && bBroadcastChangeDelegate)
	{
		OwnerComponent->BroadcastChanged();
	}
}

FGameplayTagContainer FECRCharacterPartList::CollectCombinedTags() const
{
	FGameplayTagContainer Result;

	for (const FECRAppliedCharacterPartEntry& Entry : Entries)
	{
		if (Entry.SpawnedComponent != nullptr)
		{
			if (IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(
				Entry.SpawnedComponent->GetChildActor()))
			{
				TagInterface->GetOwnedGameplayTags(/*inout*/ Result);
			}
		}
	}

	return Result;
}

void FECRCharacterPartList::UpdatePlayerStateOnEntries()
{
	for (const FECRAppliedCharacterPartEntry& Entry : Entries)
	{
		if (Entry.SpawnedComponent != nullptr)
		{
			// Set player state if it's AECRPlayerOwnedTaggedActor
			if (AECRPlayerOwnedTaggedActor* PlayerOwnedTaggedActor = Cast<AECRPlayerOwnedTaggedActor>(
				Entry.SpawnedComponent->GetChildActor()))
			{
				if (const APawn* OwningPawn = Cast<APawn>(OwnerComponent->GetOwner()))
				{
					PlayerOwnedTaggedActor->SetPlayerState(OwningPawn->GetPlayerState());
				}
			}
		}
	}
}

bool FECRCharacterPartList::SpawnActorForEntry(FECRAppliedCharacterPartEntry& Entry)
{
	bool bCreatedAnyActors = false;

	if (Entry.Part.PartClass != nullptr)
	{
		UWorld* World = OwnerComponent->GetWorld();

		if (USceneComponent* ComponentToAttachTo = OwnerComponent->GetSceneComponentToAttachTo())
		{
			const FTransform SpawnTransform = ComponentToAttachTo->GetSocketTransform(Entry.Part.SocketName);

			UChildActorComponent* PartComponent = NewObject<UChildActorComponent>(OwnerComponent->GetOwner());

			PartComponent->SetupAttachment(ComponentToAttachTo, Entry.Part.SocketName);
			PartComponent->SetChildActorClass(Entry.Part.PartClass);
			PartComponent->RegisterComponent();

			if (AActor* SpawnedActor = PartComponent->GetChildActor())
			{
				switch (Entry.Part.CollisionMode)
				{
				case ECharacterCustomizationCollisionMode::UseCollisionFromCharacterPart:
					// Do nothing
					break;

				case ECharacterCustomizationCollisionMode::NoCollision:
					SpawnedActor->SetActorEnableCollision(false);
					break;
				}

				// Set up a direct tick dependency to work around the child actor component not providing one
				if (USceneComponent* SpawnedRootComponent = SpawnedActor->GetRootComponent())
				{
					SpawnedRootComponent->AddTickPrerequisiteComponent(ComponentToAttachTo);
				}

				// Set player state if it's AECRPlayerOwnedTaggedActor
				if (AECRPlayerOwnedTaggedActor* PlayerOwnedTaggedActor = Cast<AECRPlayerOwnedTaggedActor>(
					SpawnedActor))
				{
					if (const APawn* OwningPawn = Cast<APawn>(OwnerComponent->GetOwner()))
					{
						PlayerOwnedTaggedActor->SetPlayerState(OwningPawn->GetPlayerState());
					}
				}
			}

			Entry.SpawnedComponent = PartComponent;
			bCreatedAnyActors = true;
		}
	}


	return bCreatedAnyActors;
}

bool FECRCharacterPartList::DestroyActorForEntry(FECRAppliedCharacterPartEntry& Entry)
{
	bool bDestroyedAnyActors = false;

	if (Entry.SpawnedComponent != nullptr)
	{
		Entry.SpawnedComponent->DestroyComponent();
		Entry.SpawnedComponent = nullptr;
		bDestroyedAnyActors = true;
	}

	return bDestroyedAnyActors;
}

//////////////////////////////////////////////////////////////////////

UECRPawnComponent_CharacterParts::UECRPawnComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , CharacterPartList(this)
{
	SetIsReplicatedByDefault(true);
}

void UECRPawnComponent_CharacterParts::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CharacterPartList);
	DOREPLIFETIME(ThisClass, AdditionalCosmeticTags);
}

FECRCharacterPartHandle UECRPawnComponent_CharacterParts::AddCharacterPart(const FECRCharacterPart& NewPart)
{
	return CharacterPartList.AddEntry(NewPart);
}

void UECRPawnComponent_CharacterParts::RemoveCharacterPart(FECRCharacterPartHandle Handle)
{
	CharacterPartList.RemoveEntry(Handle);
}

void UECRPawnComponent_CharacterParts::RemoveAllCharacterParts()
{
	CharacterPartList.ClearAllEntries(/*bBroadcastChangeDelegate=*/ true);
}

void UECRPawnComponent_CharacterParts::BeginPlay()
{
	Super::BeginPlay();
}

void UECRPawnComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CharacterPartList.ClearAllEntries(/*bBroadcastChangeDelegate=*/ false);

	Super::EndPlay(EndPlayReason);
}

TArray<AActor*> UECRPawnComponent_CharacterParts::GetCharacterPartActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(CharacterPartList.Entries.Num());

	for (const FECRAppliedCharacterPartEntry& Entry : CharacterPartList.Entries)
	{
		if (UChildActorComponent* PartComponent = Entry.SpawnedComponent)
		{
			if (AActor* SpawnedActor = PartComponent->GetChildActor())
			{
				Result.Add(SpawnedActor);
			}
		}
	}

	return Result;
}

USkeletalMeshComponent* UECRPawnComponent_CharacterParts::GetParentMeshComponent() const
{
	if (AActor* OwnerActor = GetOwner())
	{
		if (ACharacter* OwningCharacter = Cast<ACharacter>(OwnerActor))
		{
			if (USkeletalMeshComponent* MeshComponent = OwningCharacter->GetMesh())
			{
				return MeshComponent;
			}
		}
	}

	return nullptr;
}

USceneComponent* UECRPawnComponent_CharacterParts::GetSceneComponentToAttachTo() const
{
	if (USkeletalMeshComponent* MeshComponent = GetParentMeshComponent())
	{
		return MeshComponent;
	}
	else if (AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->GetRootComponent();
	}
	else
	{
		return nullptr;
	}
}

FGameplayTagContainer UECRPawnComponent_CharacterParts::GetCombinedTags(FGameplayTag RequiredPrefix) const
{
	FGameplayTagContainer Result = CharacterPartList.CollectCombinedTags();
	Result.AppendTags(AdditionalCosmeticTags);

	if (RequiredPrefix.IsValid())
	{
		return Result.Filter(FGameplayTagContainer(RequiredPrefix));
	}
	else
	{
		return Result;
	}
}

void UECRPawnComponent_CharacterParts::UpdatePlayerStateOnEntries()
{
	CharacterPartList.UpdatePlayerStateOnEntries();
}

void UECRPawnComponent_CharacterParts::BroadcastChanged()
{
	const bool bReinitPose = true;

	// Check to see if the body type has changed
	if (USkeletalMeshComponent* MeshComponent = GetParentMeshComponent())
	{
		// Determine the mesh to use based on cosmetic part tags
		const FGameplayTagContainer MergedTags = GetCombinedTags(FGameplayTag());
		USkeletalMesh* DesiredMesh = BodyMeshes.SelectBestBodyStyle(MergedTags);

		// Apply the desired mesh (this call is a no-op if the mesh hasn't changed)
		MeshComponent->SetSkeletalMesh(DesiredMesh, /*bReinitPose=*/ bReinitPose);

		// Apply the desired physics asset if there's a forced override independent of the one from the mesh
		if (UPhysicsAsset* PhysicsAsset = BodyMeshes.ForcedPhysicsAsset)
		{
			MeshComponent->SetPhysicsAsset(PhysicsAsset, /*bForceReInit=*/ bReinitPose);
		}
	}

	// Let observers know, e.g., if they need to apply team coloring or similar
	OnCharacterPartsChanged.Broadcast(this);
}

void UECRPawnComponent_CharacterParts::SetAdditionalCosmeticTags(const FGameplayTagContainer NewTags)
{
	AdditionalCosmeticTags = NewTags;
	OnRep_AdditionalCosmeticTags();
}

void UECRPawnComponent_CharacterParts::OnRep_AdditionalCosmeticTags()
{
	OnCharacterPartsChanged.Broadcast(this);
}
