// Copyleft: All rights reversed


#include "Gameplay/ECRCharacterAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UECRCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRCharacterAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
}

void UECRCharacterAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, Shield, OldShield)
}

void UECRCharacterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, Health, OldHealth)
}

void UECRCharacterAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRCharacterAttributeSet, Stamina, OldStamina)
}
