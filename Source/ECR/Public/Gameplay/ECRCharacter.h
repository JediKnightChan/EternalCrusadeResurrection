// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Gameplay/GAS/ECRGameplayAbility.h"
#include "GameFramework/Character.h"
#include "Gameplay/ActorAttributeComponent.h"
#include "ECRCharacter.generated.h"

UCLASS(config=Game)
class AECRCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	// -------------------------------------------------------------------
	//	GAS
	// -------------------------------------------------------------------

	/** GAS AbilitySystemComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class UECRAbilitySystemComponent* AbilitySystemComponent;

	/** GAS AttributeSet */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class UECRCharacterHealthSet* AttributeSet;

	/** Default attributes for the character (GAS Gameplay Effect) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UGameplayEffect> DefaultAttributeEffect;

	/** Default abilities for the character (GAS Gameplay Abilities) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UECRGameplayAbility>> DefaultAbilities;

	// -------------------------------------------------------------------
	//	Cameras
	// -------------------------------------------------------------------

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

protected:
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	/** Initializes GAS attributes for the character */
	virtual void InitializeAttributes();

	/** Initializes GAS abilities for the character */
	virtual void InitializeAbilities();

public:
	AECRCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	/** Initialize Ability Actor Info - workaround AbilitySystemComponent->InitAbilityActorInfo(this, this)
	 * not working and causing game crash */
	void InitAbilityActorInfo();

	/** Initialize GAS on server in PossessedBy */
	virtual void PossessedBy(AController* NewController) override;

	/** Initialize GAS on client in OnRep_PlayerState */
	virtual void OnRep_PlayerState() override;

	/** Returns AbilitySystemComponent subobject */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Setup Controller behaviour (rotate FollowCamera around ECRCharacter or change movement direction).
	 * Called from Animation Blueprint. */
	UFUNCTION(BlueprintCallable)
	void SetupControllerBehaviour(float Speed, bool bIsFalling, bool bMontageIsPlaying);
};
