// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ECRAttributeSet.h"
#include "ECRMovementSet.generated.h"

/**
 * Character movement attributes set
 * (walking speed, root motion scale, etc.).
 */
UCLASS()
class ECR_API UECRMovementSet : public UECRAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_RootMotionScale,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData RootMotionScale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_WalkSpeed,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData WalkSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Stamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_StaminaRegenRate,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData StaminaRegenRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_EvasionStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData EvasionStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxEvasionStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxEvasionStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes",
		ReplicatedUsing=OnRep_EvasionStaminaRegenDelayNormal, Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData EvasionStaminaRegenDelayNormal;

protected:
	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxShield and MaxStamina change by clamping Shield and Stamina in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes Shield, Stamina [0, MaxShield/MaxStamina], MaxShield, MaxStamina [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// RepNotifies

	UFUNCTION()
	void OnRep_RootMotionScale(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_WalkSpeed(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_EvasionStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxEvasionStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_EvasionStaminaRegenDelayNormal(const FGameplayAttributeData& OldValue) const;

public:
	UECRMovementSet();

	ATTRIBUTE_ACCESSORS(UECRMovementSet, RootMotionScale);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, WalkSpeed);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, Stamina);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, StaminaRegenRate);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, EvasionStamina);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, MaxEvasionStamina);
	ATTRIBUTE_ACCESSORS(UECRMovementSet, EvasionStaminaRegenDelayNormal);
};
