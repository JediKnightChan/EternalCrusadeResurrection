// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Components/PawnComponent.h"

#include "ECRInventoryManagerComponent.generated.h"

class UECRInventoryItemDefinition;
class UECRInventoryItemInstance;
class UECRInventoryManagerComponent;
struct FECRInventoryList;

/** A message when an item is added to the inventory */
USTRUCT(BlueprintType)
struct FECRInventoryChangeMessage
{
	GENERATED_BODY()

	//@TODO: Tag based names+owning actors for inventories instead of directly exposing the component?
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	UActorComponent* InventoryOwner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	UECRInventoryItemInstance* Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/** A single entry in an inventory */
USTRUCT(BlueprintType)
struct FECRInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FECRInventoryEntry()
	{}

	FString GetDebugString() const;

private:
	friend FECRInventoryList;
	friend UECRInventoryManagerComponent;

	UPROPERTY()
	UECRInventoryItemInstance* Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

/** List of inventory items */
USTRUCT(BlueprintType)
struct FECRInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FECRInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FECRInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	TArray<UECRInventoryItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FECRInventoryEntry, FECRInventoryList>(Entries, DeltaParms, *this);
	}

	UECRInventoryItemInstance* AddEntry(TSubclassOf<UECRInventoryItemDefinition> ItemClass, int32 StackCount);
	void AddEntry(UECRInventoryItemInstance* Instance);

	void RemoveEntry(UECRInventoryItemInstance* Instance);

private:
	void BroadcastChangeMessage(FECRInventoryEntry& Entry, int32 OldCount, int32 NewCount);

private:
	friend UECRInventoryManagerComponent;

private:
	// Replicated list of items
	UPROPERTY()
	TArray<FECRInventoryEntry> Entries;

	UPROPERTY()
	UActorComponent* OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FECRInventoryList> : public TStructOpsTypeTraitsBase2<FECRInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};


/**
 * Manages an inventory
 */
UCLASS(BlueprintType, Meta=(BlueprintSpawnableComponent))
class UECRInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UECRInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool CanAddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UECRInventoryItemInstance* AddItemDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(UECRInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(UECRInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	TArray<UECRInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UECRInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const;

	int32 GetTotalItemCountByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef) const;
	bool ConsumeItemsByDefinition(TSubclassOf<UECRInventoryItemDefinition> ItemDef, int32 NumToConsume);

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	//~End of UObject interface

private:
	UPROPERTY(Replicated)
	FECRInventoryList InventoryList;
};
