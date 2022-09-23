// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomizationMaterialAsset.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationMaterialAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	/** Namespace where loader should apply this customization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString MaterialNamespace;
	
	/** Map of Scalar Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FName, float> ScalarParameters;

	/** Map of Vector Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FName, FLinearColor> VectorParameters;

	/** Map of Texture Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FName, class UTexture*> TextureParameters;
};
