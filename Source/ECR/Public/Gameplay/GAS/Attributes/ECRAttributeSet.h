// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "ECRAttributeSet.generated.h"

/**
 * This macro defines a set of helper functions for accessing and initializing attributes.
 *
 * The following example of the macro:
 *		ATTRIBUTE_ACCESSORS(UECRHealthSet, Health)
 * will create the following functions:
 *		static FGameplayAttribute GetHealthAttribute();
 *		float GetHealth() const;
 *		void SetHealth(float NewVal);
 *		void InitHealth(float NewVal);
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

// Delegate used to broadcast attribute events.
DECLARE_MULTICAST_DELEGATE_FourParams(FECRAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/,
                                      const FGameplayEffectSpec& /*EffectSpec*/, float /*EffectMagnitude*/);

/**
 * 
 */
UCLASS()
class ECR_API UECRAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

protected:
	/** Make sure current value (eg current health) is not greater than the new max value (eg max health)
	 * when max attribute value changes */
	void ClampCurrentAttributeOnMaxChange(const FGameplayAttribute& ChangedAttribute,
	                                      const float ChangedAttributeNewValue,
	                                      const FGameplayAttribute& MaxAttribute,
	                                      const FGameplayAttribute& CurrentAttribute,
	                                      const float CurrentAttributeValue) const;

public:
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const;
};
