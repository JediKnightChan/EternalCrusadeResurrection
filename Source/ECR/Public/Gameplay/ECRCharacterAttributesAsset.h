// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ECRCharacterAttributesAsset.generated.h"

/**
 * 
 */
UCLASS()
class ECR_API UECRCharacterAttributesAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Default max shield */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DefaultMaxShield;

	/** Default max health */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DefaultMaxHealth;

	/** Default max stamina */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DefaultMaxStamina;
};
