// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRAttributeSet.h"

#include "System/ECRLogChannels.h"

void UECRAttributeSet::ClampCurrentAttributeOnMaxChange(const FGameplayAttribute& ChangedAttribute,
                                                        const float ChangedAttributeNewValue,
                                                        const FGameplayAttribute& MaxAttribute,
                                                        const FGameplayAttribute& CurrentAttribute,
                                                        const float CurrentAttributeValue) const
{
	if (ChangedAttribute == MaxAttribute)
	{
		if (CurrentAttributeValue > ChangedAttributeNewValue)
		{
			if (UECRAbilitySystemComponent* AbilitySystemComponent = GetECRAbilitySystemComponent())
			{
				AbilitySystemComponent->ApplyModToAttribute(CurrentAttribute, EGameplayModOp::Override,
				                                            ChangedAttributeNewValue);
			}
			else
			{
				UE_LOG(LogECRAbilitySystem, Error,
				       TEXT("AbilitySystemComponent invalid in UECRAttributeSet ClampCurrentAttributeOnMaxChange"))
			}
		}
	}
}


UECRAbilitySystemComponent* UECRAttributeSet::GetECRAbilitySystemComponent() const
{
	return Cast<UECRAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
