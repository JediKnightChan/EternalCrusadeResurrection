// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Weapons/ECRWeaponInstance.h"
#include "Net/UnrealNetwork.h"

UECREquipmentInstance::UECREquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UECREquipmentInstance::GetWorld() const
{
	if (APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	else
	{
		return nullptr;
	}
}

void UECREquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
	DOREPLIFETIME(ThisClass, bVisible);
}

APawn* UECREquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UECREquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}
	return Result;
}

void UECREquipmentInstance::SpawnEquipmentActors(const TArray<FECREquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		const UECRPawnComponent_CharacterParts* CosmeticComponent = OwningPawn->FindComponentByClass<
			UECRPawnComponent_CharacterParts>();
		FGameplayTagContainer CosmeticTags = {};
		if (CosmeticComponent)
		{
			CosmeticTags = CosmeticComponent->GetCombinedTags(FECRGameplayTags::Get().Cosmetic_ActorSubclass);
		}

		for (const FECREquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			TSubclassOf<AActor> ActorClass = SpawnInfo.ActorSelectionSet.SelectBestActor(CosmeticTags);
			AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(ActorClass, FTransform::Identity,
			                                                          OwningPawn);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform,
			                            SpawnInfo.AttachSocket);
			NewActor->SetActorHiddenInGame(!bVisible);
			NewActor->SetReplicates(true);

			SpawnedActors.Add(NewActor);
		}
	}
}

void UECREquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UECREquipmentInstance::OnEquipped()
{
	bEquipped = true;
	K2_OnEquipped();
}

void UECREquipmentInstance::OnUnequipped()
{
	bEquipped = false;
	K2_OnUnequipped();
}

void UECREquipmentInstance::SetVisibility(const bool bNewVisible)
{
	bVisible = bNewVisible;
	OnRep_bVisible();
}

void UECREquipmentInstance::OnRep_bVisible()
{
	for (AActor* Actor : GetSpawnedActors())
	{
		if (Actor)
		{
			Actor->SetActorHiddenInGame(!bVisible);
		}
	}

	if (bEquipped && bVisible)
	{
		if (const UECRWeaponInstance* WeaponInstance = Cast<UECRWeaponInstance>(this))
		{
			WeaponInstance->LinkAnimLayer();
		}
	}
}

void UECREquipmentInstance::OnRep_Instigator()
{
}
