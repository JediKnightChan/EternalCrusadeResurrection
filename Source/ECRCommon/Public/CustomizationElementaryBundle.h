// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "CustomizationElementaryAsset.h"
#include "Engine/DataAsset.h"
#include "CustomizationElementaryBundle.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationElementaryBundle : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Map of names to CustomizationElementaryAssets */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FString, UCustomizationElementaryAsset*> NameToElementaryAsset;
};
