// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Net/UnrealNetwork.h"


UECRCombatSet::UECRCombatSet()
	: BaseDamage(0.0f)
	  , BaseHeal(0.0f)
{
}

void UECRCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}


void UECRCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, BaseDamage, OldValue);
}


void UECRCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCombatSet, BaseHeal, OldValue);
}
