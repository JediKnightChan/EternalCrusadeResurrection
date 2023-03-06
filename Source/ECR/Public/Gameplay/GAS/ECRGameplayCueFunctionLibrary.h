#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "ECRGameplayCueFunctionLibrary.generated.h"

UCLASS()
class ECR_API UECRGameplayCueFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "GameplayCue")
	static FGameplayCueParameters MakeGameplayCueParametersFromHitResultIncludingSource(const FHitResult& HitResult);
};
