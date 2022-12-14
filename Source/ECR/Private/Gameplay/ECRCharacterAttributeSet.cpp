// Copyleft: All rights reversed


#include "Gameplay/ECRCharacterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UECRCharacterAttributeSet::UECRCharacterAttributeSet()
	: Shield(100.0f),
	  MaxShield(100.0f),
	  Stamina(100.0f),
	  MaxStamina(100.0f)
{
}


bool UECRCharacterAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		if (Data.EvaluatedData.Magnitude < 0.0f)
		{
			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(
				TAG_Gameplay_DamageSelfDestruct);

			if (Data.Target.HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any shield.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
		}
	}

	return true;
}


void UECRCharacterAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UECRCharacterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Make sure current shield is not greater than the new max shield.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxShieldAttribute(),
	                                 GetShieldAttribute(), GetShield());

	// Make sure current stamina is not greater than the new max stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxStaminaAttribute(),
	                                 GetStaminaAttribute(), GetStamina());
}


void UECRCharacterAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::ClampAttribute(Attribute, NewValue);

	if (Attribute == GetShieldAttribute())
	{
		// Do not allow shield to go negative or above max shield.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		// Do not allow max shield to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		// Do not allow stamina to go negative or above max stamina.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		// Do not allow max stamina to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UECRCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
}


void UECRCharacterAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, Shield, OldValue)
}


void UECRCharacterAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, MaxShield, OldValue)
}


void UECRCharacterAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, Stamina, OldValue)
}


void UECRCharacterAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, MaxStamina, OldValue)
}
