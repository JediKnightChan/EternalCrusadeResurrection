// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ECRDamageExecution.generated.h"

class AECRCharacter;


/**
 * 
 */
UCLASS()
class ECR_API UECRDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UECRDamageExecution();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                                    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	void ApplyDamageToCharacter(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                            FGameplayEffectCustomExecutionOutput& OutExecutionOutput,
	                            const FAggregatorEvaluateParameters EvaluateParameters,
	                            const float AttenuatedDamage) const;

	void ApplyDamageToSimpleActor(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                            FGameplayEffectCustomExecutionOutput& OutExecutionOutput,
	                            const FAggregatorEvaluateParameters EvaluateParameters,
	                            const float AttenuatedDamage) const;
};
