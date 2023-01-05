// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryManagerComponent.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Inventory_Message_StackChanged, "ECR.Inventory.Message.StackChanged");

//////////////////////////////////////////////////////////////////////
// FECRInventoryEntry

FString FECRInventoryEntry::GetDebugString() const
{
	TSubclassOf<UECRInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FECRInventoryList

void FECRInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FECRInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.StackCount, /*NewCount=*/ 0);
		Stack.LastObservedCount = 0;
	}
}

void FECRInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FECRInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ 0, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FECRInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FECRInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.LastObservedCount, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FECRInventoryList::BroadcastChangeMessage(FECRInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	FECRInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_ECR_Inventory_Message_StackChanged, Message);
}

UECRInventoryItemInstance* FECRInventoryList::AddEntry(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	UECRInventoryItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
 	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());


	FECRInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UECRInventoryItemInstance>(OwnerComponent->GetOwner());  //@TODO: Using the actor instead of component as the outer due to UE-127172
	NewEntry.Instance->SetItemDef(ItemDef);
	for (UECRInventoryItemFragment* Fragment : GetDefault<UECRInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	//const UECRInventoryItemDefinition* ItemCDO = GetDefault<UECRInventoryItemDefinition>(ItemDef);
	MarkItemDirty(NewEntry);

	return Result;
}

void FECRInventoryList::AddEntry(UECRInventoryItemInstance* Instance)
{
	unimplemented();
}

void FECRInventoryList::RemoveEntry(UECRInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FECRInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

TArray<UECRInventoryItemInstance*> FECRInventoryList::GetAllItems() const
{
	TArray<UECRInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FECRInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: Would prefer to not deal with this here and hide it further?
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

//////////////////////////////////////////////////////////////////////
// UECRInventoryManagerComponent

UECRInventoryManagerComponent::UECRInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void UECRInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

bool UECRInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	//@TODO: Add support for stack limit / uniqueness checks / etc...
	return true;
}

UECRInventoryItemInstance* UECRInventoryManagerComponent::AddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount)
{
	UECRInventoryItemInstance* Result = nullptr;
	if (ItemDef != nullptr)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);
	}
	return Result;
}

void UECRInventoryManagerComponent::AddItemInstance(UECRInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
}

void UECRInventoryManagerComponent::RemoveItemInstance(UECRInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);
}

TArray<UECRInventoryItemInstance*> UECRInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

UECRInventoryItemInstance* UECRInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const
{
	for (const FECRInventoryEntry& Entry : InventoryList.Entries)
	{
		UECRInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 UECRInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FECRInventoryEntry& Entry : InventoryList.Entries)
	{
		UECRInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool UECRInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	//@TODO: N squared right now as there's no acceleration structure
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (UECRInventoryItemInstance* Instance = UECRInventoryManagerComponent::FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

bool UECRInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FECRInventoryEntry& Entry : InventoryList.Entries)
	{
		UECRInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

//////////////////////////////////////////////////////////////////////
//

// UCLASS(Abstract)
// class UECRInventoryFilter : public UObject
// {
// public:
// 	virtual bool PassesFilter(UECRInventoryItemInstance* Instance) const { return true; }
// };

// UCLASS()
// class UECRInventoryFilter_HasTag : public UECRInventoryFilter
// {
// public:
// 	virtual bool PassesFilter(UECRInventoryItemInstance* Instance) const { return true; }
// };

