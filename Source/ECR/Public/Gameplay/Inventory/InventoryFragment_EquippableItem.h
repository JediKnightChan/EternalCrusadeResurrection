// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRInventoryItemDefinition.h"
#include "Gameplay/Equipment/ECREquipmentDefinition.h"

#include "InventoryFragment_EquippableItem.generated.h"

UCLASS()
class UInventoryFragment_EquippableItem : public UECRInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category=ECR)
	TSubclassOf<UECREquipmentDefinition> EquipmentDefinition;
};
