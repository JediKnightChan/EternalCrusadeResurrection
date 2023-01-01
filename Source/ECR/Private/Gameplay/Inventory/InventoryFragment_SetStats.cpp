// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/InventoryFragment_SetStats.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"

void UInventoryFragment_SetStats::OnInstanceCreated(UECRInventoryItemInstance* Instance) const
{
	for (const auto& KVP : InitialItemStats)
	{
		Instance->AddStatTagStack(KVP.Key, KVP.Value);
	}
}

int32 UInventoryFragment_SetStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}