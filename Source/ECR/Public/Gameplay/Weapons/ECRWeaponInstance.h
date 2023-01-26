// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Cosmetics/ECRCosmeticAnimationTypes.h"
#include "ECRWeaponInstance.generated.h"

/**
 * UECRWeaponInstance
 *
 * A piece of equipment representing a weapon spawned and applied to a pawn
 */
UCLASS()
class UECRWeaponInstance : public UECREquipmentInstance
{
	GENERATED_BODY()

public:
	UECRWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UECREquipmentInstance interface
	virtual void OnEquipped();
	virtual void OnUnequipped();
	//~End of UECREquipmentInstance interface

	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	// Returns how long it's been since the weapon was interacted with (fired or equipped)
	UFUNCTION(BlueprintPure)
	float GetTimeSinceLastInteractedWith() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FECRAnimLayerSelectionSet EquippedAnimSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Animation)
	FECRAnimLayerSelectionSet UnequippedAnimSet;

	// Choose the best layer from EquippedAnimSet or UnqeuippedAnimSet based on the specified gameplay tags
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category=Animation)
	TSubclassOf<UAnimInstance> PickBestAnimLayer(bool bEquipped, const FGameplayTagContainer& CosmeticTags) const;

protected:
	double TimeLastEquipped = 0.0;
	double TimeLastFired = 0.0;
};
