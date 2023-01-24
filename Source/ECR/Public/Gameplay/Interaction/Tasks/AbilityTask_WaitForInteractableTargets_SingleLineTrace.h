// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitForInteractableTargets.h"
#include "AbilityTask_WaitForInteractableTargets_SingleLineTrace.generated.h"

class AActor;
class UPrimitiveComponent;

UCLASS()
class UAbilityTask_WaitForInteractableTargets_SingleLineTrace : public UAbilityTask_WaitForInteractableTargets
{
	GENERATED_UCLASS_BODY()
	virtual void Activate() override;

	/** Wait until we trace new set of interactables.  This task automatically loops. */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitForInteractableTargets_SingleLineTrace* WaitForInteractableTargets_SingleLineTrace(
		UGameplayAbility* OwningAbility, FInteractionQuery InteractionQuery,
		FGameplayAbilityTargetingLocationInfo StartLocation, float InteractionScanRange = 100.0f,
		float InteractionScanRate = 0.100f, float SweepRadius = 0.0f, bool bShowDebug = false);

private:
	virtual void OnDestroy(bool AbilityEnded) override;

	void PerformTrace();

	UPROPERTY()
	FInteractionQuery InteractionQuery;

	UPROPERTY()
	FGameplayAbilityTargetingLocationInfo StartLocation;

	float InteractionScanRange = 100.0f;
	float InteractionScanRate = 0.100f;
	float SweepRadius = 0.0f;
	bool bShowDebug = false;

	FTimerHandle TimerHandle;
};
