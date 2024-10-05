// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRShieldSet.h"
#include "Net/UnrealNetwork.h"


UECRShieldSet::UECRShieldSet()
	: ShieldStamina(100.0f),
	  MaxShieldStamina(100.0f),
	  ShieldStaminaRegenRate(10.0f)
{
}


void UECRShieldSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These attributes are important only for owner
	DOREPLIFETIME_CONDITION_NOTIFY(UECRShieldSet, ShieldStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRShieldSet, MaxShieldStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRShieldSet, ShieldStaminaRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
}


void UECRShieldSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRShieldSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRShieldSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// Make sure current stamina is not greater than the new max stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxShieldStaminaAttribute(),
	                                 GetShieldStaminaAttribute(), GetShieldStamina());
}


void UECRShieldSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetShieldStaminaAttribute())
	{
		// Do not allow stamina to go negative or above max stamina.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShieldStamina());
	}
	else if (Attribute == GetMaxShieldStaminaAttribute())
	{
		// Do not allow max stamina to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UECRShieldSet::OnRep_ShieldStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRShieldSet, ShieldStamina, OldValue)
}

void UECRShieldSet::OnRep_MaxShieldStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRShieldSet, MaxShieldStamina, OldValue)
}

void UECRShieldSet::OnRep_ShieldStaminaRegenRate(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRShieldSet, ShieldStaminaRegenRate, OldValue)
}
