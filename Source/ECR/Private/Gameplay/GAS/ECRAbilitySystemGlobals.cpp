#include "Gameplay/GAS/ECRAbilitySystemGlobals.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"


UECRAbilitySystemGlobals::UECRAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FGameplayEffectContext* UECRAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FECRGameplayEffectContext();
}
