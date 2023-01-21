// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#define MAX_ALLOWED_WalkSpeed (2000.0f)

UECRCharacterHealthSet::UECRCharacterHealthSet()
	: Shield(100.0f),
	  MaxShield(100.0f),
	  BleedingHealth(100.0f),
	  MaxBleedingHealth(100.0f),
	  WalkSpeed(600.0f),
	  Stamina(100.0f),
	  MaxStamina(100.0f),
	  EvasionStamina(3.0f),
	  MaxEvasionStamina(3.0f)
{
}


void UECRCharacterHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, BleedingHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxBleedingHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, WalkSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, EvasionStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxEvasionStamina, COND_None, REPNOTIFY_Always);
}


bool UECRCharacterHealthSet::GetIsReadyToBecomeWounded() const
{
	return GetHealth() <= 0;
}

bool UECRCharacterHealthSet::GetIsReadyToDie() const
{
	return GetBleedingHealth() <= 0;
}


void UECRCharacterHealthSet::CheckIfReadyToBecomeWounded(const FGameplayEffectModCallbackData& Data)
{
	if (GetIsReadyToBecomeWounded() && !bReadyToBecomeWounded)
	{
		if (OnReadyToBecomeWounded.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
			AActor* Instigator = EffectContext.GetOriginalInstigator();
			AActor* Causer = EffectContext.GetEffectCauser();

			OnReadyToBecomeWounded.Broadcast(Instigator, Causer, Data.EffectSpec, Data.EvaluatedData.Magnitude);
		}
	}

	// Check health again in case an event above changed it.
	bReadyToBecomeWounded = GetIsReadyToBecomeWounded();
}


void UECRCharacterHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// Don't call parent function to override behaviour
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Send a standardized verb message that other systems can observe
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			SendDamageMessage(Data);
		}
		float DamageToApplyValue = GetDamage();
		if (GetShield() > 0)
		{
			// Convert into -Shield and then clamp
			const float ShieldValue = GetShield();
			SetShield(FMath::Clamp(ShieldValue - DamageToApplyValue, 0, GetMaxShield()));
			DamageToApplyValue = DamageToApplyValue - ShieldValue;
		}
		if (GetHealth() > 0 && DamageToApplyValue > 0)
		{
			// Convert into -Health and then clamp
			const float HealthValue = GetHealth();
			SetHealth(FMath::Clamp(HealthValue - DamageToApplyValue, 0, GetMaxHealth()));
			DamageToApplyValue = DamageToApplyValue - HealthValue;
		}
		if (GetBleedingHealth() > 0 && DamageToApplyValue > 0)
		{
			// Convert into -BleedingHealth and then clamp
			SetBleedingHealth(FMath::Clamp(GetBleedingHealth() - DamageToApplyValue, 0, GetMaxBleedingHealth()));
		}
		SetDamage(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		float HealingToApplyValue = GetHealing();
		if (GetBleedingHealth() < GetMaxBleedingHealth())
		{
			// Convert into +BleedingHealth and then clamp
			const float BleedingHealthValue = GetBleedingHealth();
			SetBleedingHealth(FMath::Clamp(GetBleedingHealth() + HealingToApplyValue, 0, GetMaxBleedingHealth()));
			HealingToApplyValue = HealingToApplyValue - (GetMaxBleedingHealth() - BleedingHealthValue);
		}
		if (GetHealth() < GetMaxHealth() && HealingToApplyValue > 0)
		{
			const float HealthValue = GetHealth();
			SetHealth(FMath::Clamp(GetHealth() + HealingToApplyValue, 0, GetMaxHealth()));
		}

		SetHealing(0.0f);
	}

	CheckIfReadyToBecomeWounded(Data);
	CheckIfReadyToDie(Data);
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

	// Make sure current bleeding health is not greater than the new max bleeding health.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxBleedingHealthAttribute(),
	                                 GetBleedingHealthAttribute(), GetBleedingHealth());

	// Make sure current stamina is not greater than the new max stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxStaminaAttribute(),
	                                 GetStaminaAttribute(), GetStamina());

	// Make sure current evasion stamina is not greater than the new max evasion stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxEvasionStaminaAttribute(),
	                                 GetEvasionStaminaAttribute(), GetEvasionStamina());
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
	else if (Attribute == GetBleedingHealthAttribute())
	{
		// Do not allow bleeding health to drop below 0.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxBleedingHealth());
	}
	else if (Attribute == GetMaxBleedingHealthAttribute())
	{
		// Do not allow max bleeding health to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetWalkSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, MAX_ALLOWED_WalkSpeed);
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
	else if (Attribute == GetEvasionStaminaAttribute())
	{
		// Do not allow stamina to go negative or above max evasion stamina.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEvasionStamina());
	}
	else if (Attribute == GetMaxEvasionStaminaAttribute())
	{
		// Do not allow max evasion stamina to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UECRCharacterHealthSet::OnRep_Shield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, Shield, OldValue)
}


void UECRCharacterHealthSet::OnRep_MaxShield(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxShield, OldValue)
}


void UECRCharacterHealthSet::OnRep_BleedingHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, BleedingHealth, OldValue)
}


void UECRCharacterHealthSet::OnRep_MaxBleedingHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxBleedingHealth, OldValue)
}


void UECRCharacterHealthSet::OnRep_WalkSpeed(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, WalkSpeed, OldValue)
}


void UECRCharacterHealthSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, Stamina, OldValue)
}


void UECRCharacterHealthSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxStamina, OldValue)
}

void UECRCharacterHealthSet::OnRep_EvasionStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, EvasionStamina, OldValue)
}

void UECRCharacterHealthSet::OnRep_MaxEvasionStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterHealthSet, MaxEvasionStamina, OldValue)
}
