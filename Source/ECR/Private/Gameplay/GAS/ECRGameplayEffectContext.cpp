// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"


FECRGameplayEffectContext* FECRGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(
		FECRGameplayEffectContext::StaticStruct()))
	{
		return static_cast<FECRGameplayEffectContext*>(BaseEffectContext);
	}

	return nullptr;
}


bool FECRGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}


void FECRGameplayEffectContext::SetAbilitySource(const IECRAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}


const IECRAbilitySourceInterface* FECRGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IECRAbilitySourceInterface>(AbilitySourceObject.Get());
}


const UPhysicalMaterial* FECRGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}
