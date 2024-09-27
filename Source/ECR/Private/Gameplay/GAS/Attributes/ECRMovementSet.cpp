// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRMovementSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

#define MAX_ALLOWED_WalkSpeed (2000.0f)
#define MAX_ALLOWED_RootMotionScale (10.0f)

UECRMovementSet::UECRMovementSet()
	: RootMotionScale(1.0f),
	  WalkSpeed(600.0f),
	  Stamina(100.0f),
	  MaxStamina(100.0f),
	  EvasionStamina(3.0f),
	  MaxEvasionStamina(3.0f),
	  EvasionStaminaRegenDelayNormal(5.0f)
{
}


void UECRMovementSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These attributes are important for everyone (for movement prediction)
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, RootMotionScale, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, WalkSpeed, COND_None, REPNOTIFY_Always);

	// These attributes are important only for owner
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, Stamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, MaxStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, EvasionStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, MaxEvasionStamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRMovementSet, EvasionStaminaRegenDelayNormal, COND_OwnerOnly, REPNOTIFY_Always);
}


void UECRMovementSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRMovementSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRMovementSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// Make sure current stamina is not greater than the new max stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxStaminaAttribute(),
	                                 GetStaminaAttribute(), GetStamina());

	// Make sure current evasion stamina is not greater than the new max evasion stamina.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxEvasionStaminaAttribute(),
	                                 GetEvasionStaminaAttribute(), GetEvasionStamina());
}


void UECRMovementSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetRootMotionScaleAttribute())
	{
		// Do not allow root motion scale to go negative or above max
		NewValue = FMath::Clamp(NewValue, 0.0f, MAX_ALLOWED_RootMotionScale);
	}
	else if (Attribute == GetWalkSpeedAttribute())
	{
		// Do not allow walk speed to go negative or above max
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


void UECRMovementSet::OnRep_RootMotionScale(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, RootMotionScale, OldValue)
}


void UECRMovementSet::OnRep_WalkSpeed(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, WalkSpeed, OldValue)
}


void UECRMovementSet::OnRep_Stamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, Stamina, OldValue)
}


void UECRMovementSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, MaxStamina, OldValue)
}


void UECRMovementSet::OnRep_EvasionStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, EvasionStamina, OldValue)
}


void UECRMovementSet::OnRep_MaxEvasionStamina(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, MaxEvasionStamina, OldValue)
}

void UECRMovementSet::OnRep_EvasionStaminaRegenDelayNormal(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRMovementSet, EvasionStaminaRegenDelayNormal, OldValue)
}
