// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/ECRGameplayAbilityTargetData_SingleTargetHit.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"

//////////////////////////////////////////////////////////////////////

void FECRGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(FGameplayEffectContextHandle& Context, bool bIncludeActorArray) const
{
	FGameplayAbilityTargetData_SingleTargetHit::AddTargetDataToContext(Context, bIncludeActorArray);

	// Add game-specific data
	if (FECRGameplayEffectContext* TypedContext = FECRGameplayEffectContext::ExtractEffectContext(Context))
	{
		TypedContext->CartridgeID = CartridgeID;
	}
}

bool FECRGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(Ar, Map, bOutSuccess);

	Ar << CartridgeID;

	return true;
}
