#include "Gameplay/GAS/ECRAbilitySystemFunctionLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameplayEffect.h"


struct FECRGameplayEffectContext;

FGameplayCueParameters UECRAbilitySystemFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSource(
	const FHitResult& HitResult)
{
	FGameplayCueParameters CueParameters =
		UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(HitResult);
	CueParameters.SourceObject = HitResult.GetActor();
	return CueParameters;
}

FGameplayCueParameters UECRAbilitySystemFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSourceAndCauser(
	const FHitResult& HitResult, AActor* Causer)
{
	FGameplayCueParameters CueParameters = MakeGameplayCueParametersFromHitResultIncludingSource(HitResult);
	CueParameters.EffectCauser = MakeWeakObjectPtr(Cast<AActor>(Causer));
	return CueParameters;
}

void UECRAbilitySystemFunctionLibrary::SetEffectContextSourceObject(FGameplayEffectContextHandle Handle,
                                                                    UObject* Object)
{
	Handle.AddSourceObject(Object);
}

FECRGameplayModifierInfoWrapper UECRAbilitySystemFunctionLibrary::ExtractGameplayModifierInfo(
	FGameplayModifierInfo Info)
{
	FECRGameplayModifierInfoWrapper InfoWrapper;

	InfoWrapper.Attribute = Info.Attribute;
	InfoWrapper.ModifierOp = Info.ModifierOp;
	Info.ModifierMagnitude.GetStaticMagnitudeIfPossible(1, InfoWrapper.Magnitude);
	return InfoWrapper;
}

FECRGameplayModifierInfoWrapper UECRAbilitySystemFunctionLibrary::ExtractGameplayModifierInfoFromExecution(
	FGameplayEffectExecutionDefinition Info, int32 Index)
{
	if (Index >= 0 && Index < Info.CalculationModifiers.Num())
	{
		FECRGameplayModifierInfoWrapper InfoWrapper;
		FGameplayEffectExecutionScopedModifierInfo OutInfo = Info.CalculationModifiers[Index];
		InfoWrapper.ModifierOp = OutInfo.ModifierOp;
		OutInfo.ModifierMagnitude.GetStaticMagnitudeIfPossible(1, InfoWrapper.Magnitude);
		return InfoWrapper;
	}
	return FECRGameplayModifierInfoWrapper{};
}

float UECRAbilitySystemFunctionLibrary::ExtractDurationFromGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!EffectClass)
	{
		return 0;
	}

	UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	if (!EffectCDO)
	{
		return 0;
	}

	if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		return 0;
	}
	if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Infinite)
	{
		return -1;
	}
	float OutValue = 0;
	return EffectCDO->DurationMagnitude.GetStaticMagnitudeIfPossible(1, OutValue);
}
