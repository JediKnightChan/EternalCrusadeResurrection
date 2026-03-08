// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ECRHealingExecution.generated.h"

class AECRCharacter;


/**
 * 
 */
UCLASS()
class ECR_API UECRHealingExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UECRHealingExecution();

protected:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	                                    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
