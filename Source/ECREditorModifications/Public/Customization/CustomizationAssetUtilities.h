// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Blutility/Classes/AssetActionUtility.h"
#include "CustomizationAssetUtilities.generated.h"

/**
 * 
 */
UCLASS()
class ECREDITORMODIFICATIONS_API UCustomizationAssetUtilities : public UAssetActionUtility
{
	GENERATED_BODY()

	UFUNCTION(CallInEditor)
	static void CreateElementaryAssetBundle();
};
