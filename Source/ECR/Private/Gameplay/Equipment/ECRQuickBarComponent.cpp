// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECRQuickBarComponent.h"

#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "Net/UnrealNetwork.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "Gameplay/Inventory/InventoryFragment_EquippableItem.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_QuickBar_Message_ChannelChanged, "ECR.QuickBar.Message.ChannelChanged");


FECRQuickBarChannel::FECRQuickBarChannel()
{
	if (Slots.Num() < SlotsDefaultNum)
	{
		Slots.AddDefaulted(SlotsDefaultNum - Slots.Num());
	}
}


void FECRQuickBar::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FECRQuickBarChannel& Channel = Channels[Index];
		BroadcastChangeMessage(Channel);
	}
}

void FECRQuickBar::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FECRQuickBarChannel& Channel = Channels[Index];
		BroadcastChangeMessage(Channel);
	}
}

void FECRQuickBar::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FECRQuickBarChannel& Channel = Channels[Index];
		BroadcastChangeMessage(Channel);
	}
}

void FECRQuickBar::BroadcastChangeMessage(FECRQuickBarChannel& Channel)
{
	FECRQuickBarChannelChangedMessage Message;
	Message.QuickBarOwner = OwnerComponent;
	Message.ChannelName = Channel.ChannelName;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(TAG_ECR_QuickBar_Message_ChannelChanged, Message);
}

void FECRQuickBar::UnequipItemInActiveSlot(FECRQuickBarChannel& Channel)
{
	if (UECREquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		if (Channel.EquippedItem != nullptr)
		{
			EquipmentManager->UnequipItem(Channel.EquippedItem);
			Channel.EquippedItem = nullptr;
			MarkItemDirty(Channel);
		}
	}
}

void FECRQuickBar::EquipItemInActiveSlot(FECRQuickBarChannel& Channel)
{
	check(Channel.Slots.IsValidIndex(Channel.ActiveSlotIndex));
	check(Channel.EquippedItem == nullptr);

	if (UECRInventoryItemInstance* SlotItem = Channel.Slots[Channel.ActiveSlotIndex])
	{
		if (const UInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<
			UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UECREquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				if (UECREquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
				{
					Channel.EquippedItem = EquipmentManager->EquipItem(EquipDef);
					if (Channel.EquippedItem != nullptr)
					{
						Channel.EquippedItem->SetInstigator(SlotItem);
						MarkItemDirty(Channel);
					}
				}
			}
		}
	}
}

UECREquipmentManagerComponent* FECRQuickBar::FindEquipmentManager() const
{
	if (const AController* OwnerController = Cast<AController>(OwnerComponent->GetOwner()))
	{
		if (const APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UECREquipmentManagerComponent>();
		}
	}
	return nullptr;
}

int32 FECRQuickBar::GetIndexOfChannelWithName(const FName Name) const
{
	for (int i = 0; i < Channels.Num(); i++)
	{
		if (Channels[i].ChannelName == Name)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 FECRQuickBar::GetIndexOfChannelWithNameOrCreate(const FName Name)
{
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	int32 ChannelIndex = GetIndexOfChannelWithName(Name);
	if (ChannelIndex == INDEX_NONE)
	{
		ChannelIndex = Channels.Num();

		FECRQuickBarChannel& NewChannel = Channels.AddDefaulted_GetRef();
		NewChannel.ChannelName = Name;
		MarkItemDirty(NewChannel);
	}
	return ChannelIndex;
}


UECRQuickBarComponent::UECRQuickBarComponent(const FObjectInitializer& ObjectInitializer):
	Super(ObjectInitializer),
	ChannelData(this)
{
	SetIsReplicatedByDefault(true);
}

TArray<FName> UECRQuickBarComponent::GetChannels() const
{
	TArray<FName> ChannelNames;
	for (const FECRQuickBarChannel& Channel : ChannelData.Channels)
	{
		ChannelNames.AddUnique(Channel.ChannelName);
	}
	return ChannelNames;
}

void UECRQuickBarComponent::CycleActiveSlotForward(FName ChannelName)
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return;
	}
	FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	if (Channel.Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (Channel.ActiveSlotIndex < 0 ? Channel.Slots.Num() - 1 : Channel.ActiveSlotIndex);
	int32 NewIndex = Channel.ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Channel.Slots.Num();
		if (Channel.Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(ChannelName, NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

void UECRQuickBarComponent::CycleActiveSlotBackward(FName ChannelName)
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return;
	}
	FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	if (Channel.Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (Channel.ActiveSlotIndex < 0 ? Channel.Slots.Num() - 1 : Channel.ActiveSlotIndex);
	int32 NewIndex = Channel.ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Channel.Slots.Num()) % Channel.Slots.Num();
		if (Channel.Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(ChannelName, NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

TArray<UECRInventoryItemInstance*> UECRQuickBarComponent::GetSlots(FName ChannelName) const
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return {};
	}
	return ChannelData.Channels[IndexOfChannel].Slots;
}

int32 UECRQuickBarComponent::GetActiveSlotIndex(FName ChannelName) const
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	return ChannelData.Channels[IndexOfChannel].ActiveSlotIndex;
}

UECRInventoryItemInstance* UECRQuickBarComponent::GetActiveSlotItem(FName ChannelName) const
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return nullptr;
	}
	const FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];
	return Channel.Slots.IsValidIndex(Channel.ActiveSlotIndex) ? Channel.Slots[Channel.ActiveSlotIndex] : nullptr;
}

int32 UECRQuickBarComponent::GetNextFreeItemSlot(FName ChannelName, bool bReturnZeroIfChannelMissing) const
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		if (bReturnZeroIfChannelMissing)
		{
			return 0;
		}
		return INDEX_NONE;
	}
	const FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	int32 SlotIndex = 0;
	for (TObjectPtr<UECRInventoryItemInstance> ItemPtr : Channel.Slots)
	{
		if (ItemPtr == nullptr)
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}

void UECRQuickBarComponent::AddItemToSlot(int32 SlotIndex, UECRInventoryItemInstance* Item)
{
	const FName ChannelName = Item->GetQuickBarChannelName();
	
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithNameOrCreate(ChannelName);
	FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	if (Channel.Slots.IsValidIndex(SlotIndex) && (Item != nullptr))
	{
		if (Channel.Slots[SlotIndex] == nullptr)
		{
			Channel.Slots[SlotIndex] = Item;
			ChannelData.MarkItemDirty(Channel);
		}
	}
}

UECRInventoryItemInstance* UECRQuickBarComponent::RemoveItemFromSlot(FName ChannelName, int32 SlotIndex)
{
	UECRInventoryItemInstance* Result = nullptr;

	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return Result;
	}

	FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	if (Channel.ActiveSlotIndex == SlotIndex)
	{
		ChannelData.UnequipItemInActiveSlot(Channel);
		Channel.ActiveSlotIndex = -1;
	}

	if (Channel.Slots.IsValidIndex(SlotIndex))
	{
		Result = Channel.Slots[SlotIndex];

		if (Result != nullptr)
		{
			Channel.Slots[SlotIndex] = nullptr;
		}
	}

	ChannelData.MarkItemDirty(Channel);

	return Result;
}


void UECRQuickBarComponent::SetActiveSlotIndex_Implementation(FName ChannelName, int32 NewIndex)
{
	const int32 IndexOfChannel = ChannelData.GetIndexOfChannelWithName(ChannelName);
	if (IndexOfChannel == INDEX_NONE)
	{
		return;
	}
	FECRQuickBarChannel& Channel = ChannelData.Channels[IndexOfChannel];

	if (Channel.Slots.IsValidIndex(NewIndex) && (Channel.ActiveSlotIndex != NewIndex))
	{
		ChannelData.UnequipItemInActiveSlot(Channel);
		Channel.ActiveSlotIndex = NewIndex;
		ChannelData.EquipItemInActiveSlot(Channel);
		ChannelData.MarkItemDirty(Channel);
	}
}

void UECRQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ChannelData);
}
