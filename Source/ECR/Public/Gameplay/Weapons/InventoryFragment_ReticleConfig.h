// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Inventory/ECRInventoryItemDefinition.h"
#include "GUI/Weapons/ECRReticleWidgetBase.h"

#include "InventoryFragment_ReticleConfig.generated.h"

UCLASS()
class UInventoryFragment_ReticleConfig : public UECRInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Reticle)
	TArray<TSubclassOf<UECRReticleWidgetBase>> ReticleWidgets;
};
