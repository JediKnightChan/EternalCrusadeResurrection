// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomizationElementaryAsset.h"
#include "CustomizationLoaderAsset.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationLoaderAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Array of CustomizationElementaryAssets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<UCustomizationElementaryAsset*> ElementaryAssets;

	/** Get contents of this asset as map for merging */
	FORCEINLINE TMap<FString, UCustomizationElementaryAsset*> GetAssetsAsMap()
	{
		TMap<FString, UCustomizationElementaryAsset*> Result;
		for (UCustomizationElementaryAsset* Asset : ElementaryAssets)
		{
			Result[Asset->ModuleName] = Asset;
		}
		return Result;
	}
};
