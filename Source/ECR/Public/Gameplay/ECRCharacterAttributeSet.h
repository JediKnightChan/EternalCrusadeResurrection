// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ECRHealthSet.h"
#include "ECRCharacterAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class ECR_API UECRCharacterAttributeSet : public UECRHealthSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Shield,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Shield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxShield,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxShield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Stamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxStamina;

protected:
	/** Check for damage immunity for shield in PreGameplayEffectExecute */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	
	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxShield and MaxStamina change by clamping Shield and Stamina in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes Shield, Stamina [0, MaxShield/MaxStamina], MaxShield, MaxStamina [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// RepNotifies

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;

public:
	UECRCharacterAttributeSet();

	ATTRIBUTE_ACCESSORS(UECRCharacterAttributeSet, Shield);
	ATTRIBUTE_ACCESSORS(UECRCharacterAttributeSet, MaxShield);
	ATTRIBUTE_ACCESSORS(UECRCharacterAttributeSet, Stamina);
	ATTRIBUTE_ACCESSORS(UECRCharacterAttributeSet, MaxStamina);
};
