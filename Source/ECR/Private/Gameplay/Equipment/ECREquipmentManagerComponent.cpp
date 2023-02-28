// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "System/ECRLogChannels.h"

//////////////////////////////////////////////////////////////////////
// FECRAppliedEquipmentEntry

FString FECRAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(EquipmentDefinition.Get()));
}

//////////////////////////////////////////////////////////////////////
// FECREquipmentList

void FECREquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FECRAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FECREquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FECRAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FECREquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// 	for (int32 Index : ChangedIndices)
	// 	{
	// 		const FGameplayTagStack& Stack = Stacks[Index];
	// 		TagToCountMap[Stack.Tag] = Stack.StackCount;
	// 	}
}

UECRAbilitySystemComponent* FECREquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UECRAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UECREquipmentInstance* FECREquipmentList::AddEntry(TSubclassOf<UECREquipmentDefinition> EquipmentDefinition)
{
	UECREquipmentInstance* Result = nullptr;

	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const UECREquipmentDefinition* EquipmentCDO = GetDefault<UECREquipmentDefinition>(EquipmentDefinition);

	TSubclassOf<UECREquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UECREquipmentInstance::StaticClass();
	}

	FECRAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewObject<UECREquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	//@TODO: Using the actor instead of component as the outer due to UE-127172
	Result = NewEntry.Instance;

	if (UECRAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (TObjectPtr<const UECRAbilitySet> AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, Result);
		}
	}
	else
	{
		//@TODO: Warning logging?
	}

	Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);

	MarkItemDirty(NewEntry);

	return Result;
}

void FECREquipmentList::RemoveEntry(UECREquipmentInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FECRAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			if (UECRAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}

			Instance->DestroyEquipmentActors();


			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// UECREquipmentManagerComponent

UECREquipmentManagerComponent::UECREquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UECREquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquipmentList);
}

UECREquipmentInstance* UECREquipmentManagerComponent::EquipItem(TSubclassOf<UECREquipmentDefinition> EquipmentClass)
{
	UECREquipmentInstance* Result = nullptr;
	if (EquipmentClass != nullptr)
	{
		Result = EquipmentList.AddEntry(EquipmentClass);
		if (Result != nullptr)
		{
			Result->OnEquipped();
			if (Result->bVisibleOnEquip)
			{
				SetItemVisible(Result);
			} else
			{
				Result->SetVisibility(false);
			}
		}
	}
	return Result;
}

void UECREquipmentManagerComponent::SetItemVisible(UECREquipmentInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		UE_LOG(LogECR, Warning, TEXT("Passed null into SetItemVisible"))
		return;
	}

	TArray<FName> VisibilityChannels = ItemInstance->GetVisibilityChannels();
	for (FName VisibilityChannel : VisibilityChannels)
	{
		for (const FECRAppliedEquipmentEntry Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				if (Entry.Instance->GetVisibilityChannels().Contains(VisibilityChannel))
				{
					Entry.Instance->SetVisibility(false);
				}
			}
		};
	}
	ItemInstance->SetVisibility(true);
}

void UECREquipmentManagerComponent::SetItemsInvisible(TArray<UECREquipmentInstance*> Items)
{
	for (UECREquipmentInstance* Item : Items)
	{
		if (Item)
		{
			Item->SetVisibility(false);
		}
		else
		{
			UE_LOG(LogECR, Warning, TEXT("Passed null item into SetItemsInvisible"));
		}
	}
}

void UECREquipmentManagerComponent::UnequipItem(UECREquipmentInstance* ItemInstance)
{
	if (ItemInstance != nullptr)
	{
		ItemInstance->OnUnequipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

bool UECREquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch,
                                                        FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FECRAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		UECREquipmentInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UECREquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UECREquipmentManagerComponent::UninitializeComponent()
{
	TArray<UECREquipmentInstance*> AllEquipmentInstances;

	// gathering all instances before removal to avoid side effects affecting the equipment list iterator	
	for (const FECRAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}

	for (UECREquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		UnequipItem(EquipInstance);
	}

	Super::UninitializeComponent();
}

UECREquipmentInstance* UECREquipmentManagerComponent::GetFirstInstanceOfType(
	TSubclassOf<UECREquipmentInstance> InstanceType)
{
	for (FECRAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UECREquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

TArray<UECREquipmentInstance*> UECREquipmentManagerComponent::GetEquipmentInstancesOfType(
	TSubclassOf<UECREquipmentInstance> InstanceType) const
{
	TArray<UECREquipmentInstance*> Results;
	for (const FECRAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UECREquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType))
			{
				Results.Add(Instance);
			}
		}
	}
	return Results;
}

UECREquipmentInstance* UECREquipmentManagerComponent::GetFirstInstanceVisibleInChannel(const FName VisibilityChannel)
{
	for (const FECRAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UECREquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->GetVisibilityChannels().Contains(VisibilityChannel))
			{
				if (Instance->GetIsVisible())
				{
					return Instance;
				}
			}
		}
	}

	return nullptr;
}
