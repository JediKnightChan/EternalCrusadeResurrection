// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRHealthComponent.h"
#include "ECRCharacterHealthComponent.generated.h"


class UECRCharacterHealthSet;

/**
 * UECRCharacterHealthComponent
 *
 *	An actor component used to handle anything related to character health.
 */
UCLASS()
class ECR_API UECRCharacterHealthComponent : public UECRHealthComponent
{
	GENERATED_BODY()

public:
	// Initialize the component using an ability system component.
	virtual void InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC) override;

	// Uninitialize the component, clearing any references to the ability system.
	virtual void UninitializeFromAbilitySystem() override;

	// Returns the current shield value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Shield")
	float GetShield() const;

	// Returns the current maximum shield value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Shield")
	float GetMaxShield() const;

	// Returns the current shield in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "ECR|Shield")
	float GetShieldNormalized() const;

	// Returns the current bleeding health value.
	UFUNCTION(BlueprintCallable, Category = "ECR|BleedingHealth")
	float GetBleedingHealth() const;

	// Returns the current bleeding health value.
	UFUNCTION(BlueprintCallable, Category = "ECR|BleedingHealth")
	float GetMaxBleedingHealth() const;

	// Returns the current bleeding health in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "ECR|BleedingHealth")
	float GetBleedingHealthNormalized() const;

	// Returns the current stamina value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetStamina() const;

	// Returns the current maximum stamina value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetMaxStamina() const;

	// Returns the current stamina in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetStaminaNormalized() const;

	// Returns the current evasion stamina value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetEvasionStamina() const;

	// Returns the current evasion stamina value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetMaxEvasionStamina() const;

	// Returns the current evasion stamina value in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "ECR|Stamina")
	float GetEvasionStaminaNormalized() const;

public:
	// Delegate fired when the shield value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnShieldChanged;

	// Delegate fired when the max shield value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnMaxShieldChanged;

	// Delegate fired when the bleeding health value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnBleedingHealthChanged;

	// Delegate fired when the max bleeding health value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnMaxBleedingHealthChanged;

	// Delegate fired when the stamina value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnStaminaChanged;

	// Delegate fired when the max stamina value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnMaxStaminaChanged;

	// Delegate fired when the evasion stamina value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnEvasionStaminaChanged;

	// Delegate fired when the max evasion stamina value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnMaxEvasionStaminaChanged;

protected:
	virtual void ClearGameplayTags() override;

	void ChangeCharacterSpeed(float NewSpeed);

	virtual void HandleWalkSpeedChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxShieldChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleBleedingHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxBleedingHealthChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleReadyToBecomeWounded(AActor* DamageInstigator, AActor* DamageCauser,
	                                        const FGameplayEffectSpec& DamageEffectSpec, float DamageMagnitude);

	virtual void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData);

	virtual float GetDamageToKill() override;
protected:
	// Health set used by this component. Duplicates HealthSet.
	UPROPERTY()
	const UECRCharacterHealthSet* CharacterHealthSet;
};
