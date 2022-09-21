// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "CustomizationAssetUtilities.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationAssetUtilities : public UAssetActionUtility
{
	GENERATED_BODY()

	UFUNCTION(CallInEditor)
	static void CreateElementaryAssetBundle();
};
