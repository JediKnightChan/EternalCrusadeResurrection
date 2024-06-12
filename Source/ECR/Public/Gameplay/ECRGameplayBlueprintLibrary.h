// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ECRGameplayBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ECR_API UECRGameplayBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static double
	CalculateDamageAttenuationForArmorPenetration(double ArmorPenetration, double Toughness, double Armor);

	UFUNCTION(BlueprintCallable, blueprintPure)
	static FVector VRandConeNormalDistribution(const FVector& Dir, const float ConeHalfAngleRad, const float Exponent);
};
