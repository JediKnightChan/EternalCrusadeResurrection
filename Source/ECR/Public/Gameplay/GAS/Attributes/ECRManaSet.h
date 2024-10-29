// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ECRAttributeSet.h"
#include "ECRManaSet.generated.h"

/**
 * Mana set
 * (for sorcerers).
 */
UCLASS()
class ECR_API UECRManaSet : public UECRAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Mana,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Mana;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxMana,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxMana;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_ManaRegenRate,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData ManaRegenRate;

protected:
	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxMana change by clamping Mana in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes Mana [0, MaxMana], MaxMana [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// RepNotifies
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue) const;
public:
	UECRManaSet();
	
	ATTRIBUTE_ACCESSORS(UECRManaSet, Mana);
	ATTRIBUTE_ACCESSORS(UECRManaSet, MaxMana);
	ATTRIBUTE_ACCESSORS(UECRManaSet, ManaRegenRate);
};
