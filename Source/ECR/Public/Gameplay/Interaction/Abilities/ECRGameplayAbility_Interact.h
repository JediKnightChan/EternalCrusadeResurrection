// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "ECRGameplayAbility_Interact.generated.h"

class UIndicatorDescriptor;

/**
 * UECRGameplayAbility_Interact
 *
 * Gameplay ability used for character interacting
 */
UCLASS(Abstract)
class UECRGameplayAbility_Interact : public UECRGameplayAbility
{
	GENERATED_BODY()

public:

	UECRGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable)
	void UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions);

	UFUNCTION(BlueprintCallable)
	void TriggerInteraction();

protected:
	UPROPERTY(BlueprintReadWrite)
	TArray<FInteractionOption> CurrentOptions;

	UPROPERTY()
	TArray<UIndicatorDescriptor*> Indicators;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	float InteractionScanRate = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	float InteractionScanRange = 500;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UUserWidget> DefaultInteractionWidgetClass;
};
