#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "ECRAbilitySystemFunctionLibrary.generated.h"

UCLASS()
class ECR_API UECRAbilitySystemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GameplayCue")
	static FGameplayCueParameters MakeGameplayCueParametersFromHitResultIncludingSource(const FHitResult& HitResult);
	
	UFUNCTION(BlueprintPure, Category = "GameplayCue")
    static FGameplayCueParameters MakeGameplayCueParametersFromHitResultIncludingSourceAndCauser(const FHitResult& HitResult, AActor* Causer);

	UFUNCTION(BlueprintCallable, Category = "GameplayEffect")
	static void SetEffectContextSourceObject(FGameplayEffectContextHandle Handle, UObject* Object);
};
