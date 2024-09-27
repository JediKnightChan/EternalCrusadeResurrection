#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "ECRAbilitySystemFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FECRGameplayModifierInfoWrapper
{
	GENERATED_BODY()

	// GAS Attribute
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayAttribute Attribute;

	// Modifier type
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EGameplayModOp::Type> ModifierOp;

	// Magnitude
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Magnitude;
};


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
	
	UFUNCTION(BlueprintPure)
	static FECRGameplayModifierInfoWrapper ExtractGameplayModifierInfo(FGameplayModifierInfo Info);
};
