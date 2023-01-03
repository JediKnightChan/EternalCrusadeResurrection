// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"
#include "ECRGameplayAbility_FromEquipment.generated.h"

class UECREquipmentInstance;
class UECRInventoryItemInstance;

/**
 * UECRGameplayAbility_FromEquipment
 *
 * An ability granted by and associated with an equipment instance
 */
UCLASS()
class UECRGameplayAbility_FromEquipment : public UECRGameplayAbility
{
	GENERATED_BODY()

public:

	UECRGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="ECR|Ability")
	UECREquipmentInstance* GetAssociatedEquipment(UObject* SourceObject = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Ability")
	UECRInventoryItemInstance* GetAssociatedItem(UObject* SourceObject = nullptr) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

};
