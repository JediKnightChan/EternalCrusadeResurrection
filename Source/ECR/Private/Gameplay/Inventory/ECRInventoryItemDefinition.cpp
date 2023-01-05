// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"

//////////////////////////////////////////////////////////////////////
// UECRInventoryItemDefinition

UECRInventoryItemDefinition::UECRInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UECRInventoryItemFragment* UECRInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<UECRInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UECRInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// UECRInventoryItemDefinition

const UECRInventoryItemFragment* UECRInventoryFunctionLibrary::FindItemDefinitionFragment(
	TSubclassOf<UECRInventoryItemDefinition> ItemDef, TSubclassOf<UECRInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UECRInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}
