// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "ECRQuickBarComponent.generated.h"

class UECRInventoryItemInstance;
class UECREquipmentInstance;
class UECREquipmentManagerComponent;

/** A single channel in quickbar */
USTRUCT(BlueprintType)
struct FECRQuickBarChannel : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FECRQuickBarChannel();

protected:
	int SlotsDefaultNum = 3;

private:
	friend struct FECRQuickBar;
	friend UECRQuickBarComponent;

	UPROPERTY()
	FName ChannelName;

	UPROPERTY()
	TArray<TObjectPtr<UECRInventoryItemInstance>> Slots;

	UPROPERTY()
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	TObjectPtr<UECREquipmentInstance> EquippedItem = nullptr;
};


/** List of inventory items */
USTRUCT(BlueprintType)
struct FECRQuickBar : public FFastArraySerializer
{
	GENERATED_BODY()

	FECRQuickBar()
		: OwnerComponent(nullptr)
	{
	}

	FECRQuickBar(UControllerComponent* InOwnerComponent)
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
		return FFastArraySerializer::FastArrayDeltaSerialize<FECRQuickBarChannel, FECRQuickBar>(
			Channels, DeltaParms, *this);
	}

private:
	void BroadcastChangeMessage(FECRQuickBarChannel& Channel);
	void UnequipItemInActiveSlot(FECRQuickBarChannel& Channel);
	void EquipItemInActiveSlot(FECRQuickBarChannel& Channel);

	UECREquipmentManagerComponent* FindEquipmentManager() const;

	int32 GetIndexOfChannelWithName(FName Name) const;
	int32 GetIndexOfChannelWithNameOrCreate(FName Name);

private:
	friend UECRQuickBarComponent;

private:
	// Replicated list of channels
	UPROPERTY()
	TArray<FECRQuickBarChannel> Channels;

	UPROPERTY()
	UControllerComponent* OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FECRQuickBar> : public TStructOpsTypeTraitsBase2<FECRQuickBar>
{
	enum { WithNetDeltaSerializer = true };
};


UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class UECRQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UECRQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FName> GetChannels() const;

	UFUNCTION(BlueprintCallable, Category="ECR")
	void CycleActiveSlotForward(FName ChannelName);

	UFUNCTION(BlueprintCallable, Category="ECR")
	void CycleActiveSlotBackward(FName ChannelName);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="ECR")
	void SetActiveSlotIndex(FName ChannelName, int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UECRInventoryItemInstance*> GetSlots(FName ChannelName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex(FName ChannelName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UECRInventoryItemInstance* GetActiveSlotItem(FName ChannelName) const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot(FName ChannelName, bool bReturnZeroIfChannelMissing) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, UECRInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UECRInventoryItemInstance* RemoveItemFromSlot(FName ChannelName, int32 SlotIndex);

private:
	UPROPERTY(Replicated)
	FECRQuickBar ChannelData;
};


USTRUCT(BlueprintType)
struct FECRQuickBarChannelChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Quickbar)
	FName ChannelName;

	UPROPERTY(BlueprintReadOnly, Category=Quickbar)
	UControllerComponent* QuickBarOwner = nullptr;
};
