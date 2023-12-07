// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UECRCharacterHealthSet::UECRCharacterHealthSet()
	: Shield(100.0f),
	  MaxShield(100.0f),
	  BleedingHealth(100.0f),
	  MaxBleedingHealth(100.0f)
{
	bLastTimeWasWounded = false;
}


void UECRCharacterHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, BleedingHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterHealthSet, MaxBleedingHealth, COND_None, REPNOTIFY_Always);
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
	if (GetIsReadyToBecomeWounded())
	{
		if (!bLastTimeWasWounded)
		{
			bLastTimeWasWounded = true;

			if (OnReadyToBecomeWounded.IsBound())
			{
				const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
				AActor* Instigator = EffectContext.GetOriginalInstigator();
				AActor* Causer = EffectContext.GetEffectCauser();

				OnReadyToBecomeWounded.Broadcast(Instigator, Causer, Data.EffectSpec, Data.EvaluatedData.Magnitude);
			}
		}
	}
	else
	{
		if (bLastTimeWasWounded)
		{
			bLastTimeWasWounded = false;
			
			if (OnReadyToBecomeUnwounded.IsBound())
			{
				const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
				AActor* Instigator = EffectContext.GetOriginalInstigator();
				AActor* Causer = EffectContext.GetEffectCauser();

				OnReadyToBecomeUnwounded.Broadcast(Instigator, Causer, Data.EffectSpec, Data.EvaluatedData.Magnitude);
			}
		}
	}
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
	// Make sure current health is not greater than the new max health.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxHealthAttribute(),
	                                 GetHealthAttribute(), GetHealth());

	// Make sure current shield is not greater than the new max shield.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxShieldAttribute(),
	                                 GetShieldAttribute(), GetShield());

	// Make sure current bleeding health is not greater than the new max bleeding health.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxBleedingHealthAttribute(),
	                                 GetBleedingHealthAttribute(), GetBleedingHealth());
	if (bReadyToDie && !GetIsReadyToDie())
	{
		bReadyToDie = false;
	}
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
