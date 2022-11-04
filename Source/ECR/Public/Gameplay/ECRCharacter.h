// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ECRCharacter.generated.h"

UCLASS(config=Game)
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class AECRCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Attributes asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class UECRCharacterAttributesAsset* AttributesAsset;

	/** Health component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay, meta = (AllowPrivateAccess = "true"))
	class UActorAttributeComponent* HealthComponent;

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

	/** Process health change of character */
	void ProcessHealthChange(const float NewHealth, const float MaxHealth);

	// GUI Blueprint Implementable Events
	UFUNCTION(BlueprintImplementableEvent)
	void GUIProcessHealthChange(float NewHealth, float MaxHealth);


public:
	AECRCharacter();

	virtual void BeginPlay() override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Input)
	float TurnRateGamepad;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Setup Controller behaviour (rotate FollowCamera around ECRCharacter or change movement direction).
	 * Called from Animation Blueprint. */
	UFUNCTION(BlueprintCallable)
	void SetupControllerBehaviour(float Speed, bool bIsFalling, bool bMontageIsPlaying);
};
