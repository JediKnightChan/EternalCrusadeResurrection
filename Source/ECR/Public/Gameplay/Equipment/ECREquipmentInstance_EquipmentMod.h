// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECREquipmentInstance.h"

#include "ECREquipmentInstance_EquipmentMod.generated.h"


/**
 * UECREquipmentInstance_EquipmentMod
 *
 * An item which goal is to modify another item (eg weapon handle which increases weapon accuracy)
 */
UCLASS(BlueprintType, Blueprintable)
class UECREquipmentInstance_EquipmentMod : public UECREquipmentInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	/** Apply this modifier to equipment instance (eg change its properties) */
	void ModifyWantedEquipmentInstance(UECREquipmentInstance* Instance);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer ModifierTags;
};
