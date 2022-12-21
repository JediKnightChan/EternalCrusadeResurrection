// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UECRCharacterHealthSet::UECRCharacterHealthSet()
	: Shield(100.0f),
	  MaxShield(100.0f),
	  Stamina(100.0f),
	  MaxStamina(100.0f)
{
}


bool UECRCharacterHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
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


void UECRCharacterHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRCharacterHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UECRCharacterHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Make sure current shield is not greater than the new max shield.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxShieldAttribute(),
	                                 GetShieldAttribute(), GetShield());

	// Make sure current stamina is not greater than the new max stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxStaminaAttribute(),
	                                 GetStaminaAttribute(), GetStamina());
}


void UECRCharacterHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
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


void UECRCharacterHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxStamina, COND_None, REPNOTIFY_Always);
}


void UECRCharacterHealthSet::OnRep_Shield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, Shield, OldValue)
}


void UECRCharacterHealthSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxShield, OldValue)
}


void UECRCharacterHealthSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, Stamina, OldValue)
}


void UECRCharacterHealthSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxStamina, OldValue)
}
