// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Character/ECRPawnComponent.h"
#include "Gameplay/Camera/ECRCameraMode.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Input/ECRMappableConfigPair.h"
#include "ECRPawnControlComponent.generated.h"


class AECRPlayerState;
class UInputComponent;
struct FInputActionValue;
class UECRInputConfig;
class UECRInputComponent;

/**
 * UECRPawnControlComponent
 *
 *	A component used to create a player controlled pawns (characters, vehicles, etc..).
 */
UCLASS(Abstract, Blueprintable, Meta=(BlueprintSpawnableComponent))
class UECRPawnControlComponent : public UECRPawnComponent
{
	GENERATED_BODY()

public:
	UECRPawnControlComponent(const FObjectInitializer& ObjectInitializer);

	// Returns the hero component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "ECR|Hero")
	static UECRPawnControlComponent* FindPawnControlComonent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<UECRPawnControlComponent>() : nullptr);
	}

	void SetAbilityCameraMode(TSubclassOf<UECRCameraMode> CameraMode,
	                          const FGameplayAbilitySpecHandle& OwningSpecHandle);
	void ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle);

	void AddAdditionalInputConfig(const UECRInputConfig* InputConfig);
	void RemoveAdditionalInputConfig(const UECRInputConfig* InputConfig);

	/** Enables or disables movement input (movement, crouch, auto run) */
	void ToggleMovementInput(bool bNewEnabled);

	/** True if this has completed OnPawnReadyToInitialize so is prepared for late-added features */
	bool HasPawnInitialized() const;

	/** True if this player has sent the BindInputsNow event and is prepared for bindings */
	bool IsReadyToBindInputs() const;

	static const FName NAME_BindInputsNow;

protected:
	virtual void OnRegister() override;

	virtual bool IsPawnComponentReadyToInitialize() const override;
	virtual void OnPawnReadyToInitialize();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	virtual void BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig);

	void NotifyAbilityQueueSystem(UECRAbilitySystemComponent* ASC, const FGameplayTag& InputTag);

	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	virtual void Input_LookMouse(const FInputActionValue& InputActionValue);
	virtual void Input_LookStick(const FInputActionValue& InputActionValue);

	TSubclassOf<UECRCameraMode> DetermineCameraMode() const;

protected:
	/** List of input tags handled by ability queue system */
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer AbilityQueueInputTags;

	/**
	 * Input Configs that should be added to this player when initalizing the input.
	 * 
	 * NOTE: You should only add to this if you do not have a game feature plugin accessible to you.
	 * If you do, then use the GameFeatureAction_AddInputConfig instead. 
	 */
	UPROPERTY(EditAnywhere)
	TArray<FMappableConfigPair> DefaultInputConfigs;

	// Camera mode set by an ability.
	TSubclassOf<UECRCameraMode> AbilityCameraMode;

	// Spec handle for the last ability to set a camera mode.
	FGameplayAbilitySpecHandle AbilityCameraModeOwningSpecHandle;

	// True when the pawn has fully finished initialization
	bool bPawnHasInitialized;

	// True when player input bindings have been applied, will never be true for non-players
	bool bReadyToBindInputs;

	// True if movement input enabled (by default)
	bool bMovementInputEnabled;

public:
	// True when should listen for ability queue
	bool bListenForAbilityQueue;

	// Delta time for ability queue system
	double AbilityQueueDeltaTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double LookPitchLimit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double LookYawLimit;
};
