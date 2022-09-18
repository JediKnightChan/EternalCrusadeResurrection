// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomizationMaterialAsset.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationMaterialAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** Namespace where loader should apply this customization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString MaterialNamespace;
	
	/** Map of Scalar Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, float> ScalarParameters;

	/** Map of Vector Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, FLinearColor> VectorParameters;

	/** Map of Texture Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, class UTexture*> TextureParameters;
};
