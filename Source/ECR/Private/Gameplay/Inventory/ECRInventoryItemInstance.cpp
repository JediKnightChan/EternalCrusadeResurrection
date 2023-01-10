// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"

UECRInventoryItemInstance::UECRInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UECRInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

void UECRInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void UECRInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 UECRInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool UECRInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void UECRInventoryItemInstance::SetItemDef(TSubclassOf<UECRInventoryItemDefinition> InDef)
{
	ItemDef = InDef;

	const UECRInventoryItemDefinition* ItemDefCDO = InDef.GetDefaultObject();
	QuickBarChannelName = ItemDefCDO->QuickBarChannelName;
}

const UECRInventoryItemFragment* UECRInventoryItemInstance::FindFragmentByClass(
	TSubclassOf<UECRInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UECRInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}
