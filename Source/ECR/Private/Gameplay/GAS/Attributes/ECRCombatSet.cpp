// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Net/UnrealNetwork.h"


UECRCombatSet::UECRCombatSet()
	: BaseDamage(0.0f)
	  , BaseHeal(0.0f)
	  , Toughness(100.0f)
	  , IncomingDamageMultiplier(1.0f)
	  , Armor(100.0f),
	  RecoilMultiplier(1.0f)
{
}

void UECRCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These attributes are only for owner
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, RecoilMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, Toughness, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, IncomingDamageMultiplier, COND_OwnerOnly, REPNOTIFY_Always);

	// These attributes are important for everyone
	// Armor is used to draw non penetration marker when confirming hits
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, Armor, COND_None, REPNOTIFY_Always);
}


void UECRCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, BaseDamage, OldValue);
}


void UECRCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, BaseHeal, OldValue);
}

void UECRCombatSet::OnRep_Toughness(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, Toughness, OldValue);
}

void UECRCombatSet::OnRep_IncomingDamageMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, IncomingDamageMultiplier, OldValue);
}

void UECRCombatSet::OnRep_Armor(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, Armor, OldValue);
}

void UECRCombatSet::OnRep_RecoilMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, RecoilMultiplier, OldValue);
}
