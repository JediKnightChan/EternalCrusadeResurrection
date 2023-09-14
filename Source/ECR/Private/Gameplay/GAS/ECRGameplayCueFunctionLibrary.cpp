#include "Gameplay/GAS/ECRGameplayCueFunctionLibrary.h"
#include "GameplayCueFunctionLibrary.h"


FGameplayCueParameters UECRGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSource(
	const FHitResult& HitResult)
{
	FGameplayCueParameters CueParameters = UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(HitResult);
	CueParameters.SourceObject = HitResult.GetActor();
	return CueParameters;
}

FGameplayCueParameters UECRGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSourceAndCauser(
	const FHitResult& HitResult, AActor* Causer)
{
	FGameplayCueParameters CueParameters = MakeGameplayCueParametersFromHitResultIncludingSource(HitResult);
	CueParameters.EffectCauser = MakeWeakObjectPtr(Cast<AActor>(Causer));
	return CueParameters;
}
