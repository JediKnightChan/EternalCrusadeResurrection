// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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
	TArray<class UCustomizationElementaryAsset*> ElementaryAssets;
};
