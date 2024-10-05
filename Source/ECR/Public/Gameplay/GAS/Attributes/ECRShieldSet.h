// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ECRAttributeSet.h"
#include "ECRShieldSet.generated.h"

/**
 * Shield set
 * (for shield wielders).
 */
UCLASS()
class ECR_API UECRShieldSet : public UECRAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_ShieldStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ShieldStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxShieldStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxShieldStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_ShieldStaminaRegenRate,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ShieldStaminaRegenRate;

protected:
	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxShieldStamina change by clamping ShieldStamina in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes ShieldStamina [0, MaxShieldStamina], MaxShieldStamina [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// RepNotifies
	
	UFUNCTION()
	void OnRep_ShieldStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxShieldStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_ShieldStaminaRegenRate(const FGameplayAttributeData& OldValue) const;
public:
	UECRShieldSet();
	
	ATTRIBUTE_ACCESSORS(UECRShieldSet, ShieldStamina);
	ATTRIBUTE_ACCESSORS(UECRShieldSet, MaxShieldStamina);
	ATTRIBUTE_ACCESSORS(UECRShieldSet, ShieldStaminaRegenRate);
};
