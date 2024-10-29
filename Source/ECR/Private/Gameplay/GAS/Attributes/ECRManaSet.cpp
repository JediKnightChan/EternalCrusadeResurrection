// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRManaSet.h"
#include "Net/UnrealNetwork.h"


UECRManaSet::UECRManaSet()
	: Mana(100.0f),
	  MaxMana(100.0f),
	  ManaRegenRate(20.0f)
{
}


void UECRManaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These attributes are important only for owner
	DOREPLIFETIME_CONDITION_NOTIFY(UECRManaSet, Mana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRManaSet, MaxMana, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRManaSet, ManaRegenRate, COND_OwnerOnly, REPNOTIFY_Always);
}


void UECRManaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRManaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRManaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	// Make sure current mana is not greater than the new max mana.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxManaAttribute(),
	                                 GetManaAttribute(), GetMana());
}


void UECRManaSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetManaAttribute())
	{
		// Do not allow mana to go negative or above max mana.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		// Do not allow max mana to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UECRManaSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRManaSet, Mana, OldValue)
}

void UECRManaSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRManaSet, MaxMana, OldValue)
}

void UECRManaSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRManaSet, ManaRegenRate, OldValue)
}
