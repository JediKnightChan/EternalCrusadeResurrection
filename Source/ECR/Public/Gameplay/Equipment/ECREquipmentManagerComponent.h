// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Components/PawnComponent.h"

#include "ECREquipmentManagerComponent.generated.h"

class UECREquipmentDefinition;
class UECREquipmentInstance;
class UECRAbilitySystemComponent;
struct FECREquipmentList;
class UECREquipmentManagerComponent;

/** A single piece of applied equipment */
USTRUCT(BlueprintType)
struct FECRAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FECRAppliedEquipmentEntry()
	{
	}

	FString GetDebugString() const;

private:
	friend FECREquipmentList;
	friend UECREquipmentManagerComponent;

	// The equipment class that got equipped
	UPROPERTY()
	TSubclassOf<UECREquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	UECREquipmentInstance* Instance = nullptr;

	// Authority-only list of granted handles
	UPROPERTY(NotReplicated)
	FECRAbilitySet_GrantedHandles GrantedHandles;
};

/** List of applied equipment */
USTRUCT(BlueprintType)
struct FECREquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FECREquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FECREquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FECRAppliedEquipmentEntry, FECREquipmentList>(
			Entries, DeltaParms, *this);
	}

	UECREquipmentInstance* AddEntry(TSubclassOf<UECREquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(UECREquipmentInstance* Instance);

private:
	UECRAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UECREquipmentManagerComponent;

private:
	// Replicated list of equipment entries
	UPROPERTY()
	TArray<FECRAppliedEquipmentEntry> Entries;

	UPROPERTY()
	UActorComponent* OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FECREquipmentList> : public TStructOpsTypeTraitsBase2<FECREquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};


/**
 * Manages equipment applied to a pawn
 */
UCLASS(BlueprintType, Const, Meta=(BlueprintSpawnableComponent))
class UECREquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UECREquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UECREquipmentInstance* EquipItem(TSubclassOf<UECREquipmentDefinition> EquipmentDefinition);

	UFUNCTION(BlueprintCallable)
	void SetItemVisible(UECREquipmentInstance* ItemInstance);

	UFUNCTION(BlueprintCallable)
	static void SetItemsInvisible(TArray<UECREquipmentInstance*> Items);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UnequipItem(UECREquipmentInstance* ItemInstance);

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch,
	                                 FReplicationFlags* RepFlags) override;
	//~End of UObject interface

	//~UActorComponent interface
	//virtual void EndPlay() override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	//~End of UActorComponent interface

	/** Returns the first equipped instance of a given type, or nullptr if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UECREquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UECREquipmentInstance> InstanceType);

	/** Returns all equipped instances of a given type, or an empty array if none are found */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UECREquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UECREquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

	/** Returns the first equipped instance which is visible and belongs to the specified visibility channel */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UECREquipmentInstance* GetFirstInstanceVisibleInChannel(FName VisibilityChannel);

private:
	UPROPERTY(Replicated)
	FECREquipmentList EquipmentList;
};
