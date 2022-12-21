// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "ECR.h"
#include "Abilities/GameplayAbility.h"
#include "ECRGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class ECR_API UECRGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UECRGameplayAbility();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	EECRAbilityInputID AbilityInputID;
};
