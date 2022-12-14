// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CustomizationSavingNameSpace.generated.h"


/** All data regarding one CustomizationMaterialNamespace */
USTRUCT(BlueprintType)
struct ECRCOMMON_API FCustomizationMaterialNamespaceData
{
	GENERATED_BODY()

	FCustomizationMaterialNamespaceData()
	{
		RelativeSavePath = "";
		ScalarParameters = {};
		VectorParameters = {};
		TextureParameters = {};
	}

	/** Whether it does not contain any parameters */
	bool IsEmpty() const
	{
		return (ScalarParameters.Num() == 0 && VectorParameters.Num() == 0 && TextureParameters.Num() == 0);
	}
	
	/** Relative path in CustomizationSavingNameSpace root directory to save asset to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString RelativeSavePath;

	/** Map of Scalar Material Parameter Name to its Value */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, float> ScalarParameters;

	/** Map of Vector Material Parameter Name to its Value */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, FLinearColor> VectorParameters;

	/** Map of Texture Material Parameter Name to its Value */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, class UTexture*> TextureParameters;
};


UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationSavingNameSpace : public USceneComponent
{
	GENERATED_BODY()

	/** Save every child CustomizationElementaryModule, overwriting / skipping it if it already exists,
	 * and produce CustomizationLoaderAsset */
	void SaveLoadout(bool bDoOverwrite);
public:
	// Sets default values for this component's properties
	UCustomizationSavingNameSpace();

	/* Save destination root directory for customization assets (eg, /Game/Characters/SpaceMarine/Customization/) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString SaveDestinationRootDirectory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FString, FCustomizationMaterialNamespaceData> MaterialCustomizationData;

	/** Save every child CustomizationElementaryModule, overwriting if it already exists,
	 * and produce CustomizationLoaderAsset */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveLoadoutOverwritingExistingModules();

	/** Save every child CustomizationElementaryModule, skipping if it already exists,
	 * and produce CustomizationLoaderAsset */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveLoadoutSkippingExistingModules();
	void SaveMaterialCustomizationData(bool bDoOverwrite) const;
};
