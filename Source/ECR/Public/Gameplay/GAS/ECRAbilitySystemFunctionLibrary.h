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

	UFUNCTION(BlueprintPure)
	static FECRGameplayModifierInfoWrapper ExtractGameplayModifierInfoFromExecution(FGameplayEffectExecutionDefinition Info, int32 Index);

	UFUNCTION(BlueprintPure)
	static float ExtractDurationFromGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass);

	/** Returns the UI data for a gameplay effect class (if any) */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayAbility", Meta = (DisplayName = "Get GameplayAbility UI Data", DeterminesOutputType="DataType"))
	static const UGameplayEffectUIData* GetGameplayAbilityUIData(TSubclassOf<UECRGameplayAbility> AbilityClass, TSubclassOf<UGameplayEffectUIData> DataType);

	UFUNCTION(BlueprintPure)
	static UObject* ExtractSourceFromEffectHandle(FActiveGameplayEffectHandle Handle);

	UFUNCTION(BlueprintPure)
	static UObject* ExtractInstigatorFromEffectHandle(FActiveGameplayEffectHandle Handle);

	UFUNCTION(BlueprintPure)
	static UObject* ExtractEffectCauserFromEffectHandle(FActiveGameplayEffectHandle Handle);

	UFUNCTION(BlueprintPure)
	static bool GetIsAbilityActive(UAbilitySystemComponent* AbilitySystem, FGameplayAbilitySpecHandle Handle);

	UFUNCTION(BlueprintPure)
	static bool GetIsAbilityActivatable(UAbilitySystemComponent* AbilitySystem, FGameplayAbilitySpecHandle Handle);

	UFUNCTION(BlueprintPure)
	static float GetAbilityTotalCooldown(UAbilitySystemComponent* AbilitySystem, FGameplayAbilitySpecHandle Handle);
};
