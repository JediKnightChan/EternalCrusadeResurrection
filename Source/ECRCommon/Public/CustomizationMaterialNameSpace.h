// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstance.h"
#include "CustomizationMaterialNameSpace.generated.h"


UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationMaterialNameSpace : public USceneComponent
{
	GENERATED_BODY()

	/** Relative path in CustomizationSavingNameSpace root directory to save asset to */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString RelativeSavePath;
	
	/** Map of Scalar Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, float> ScalarParameters;

	/** Map of Vector Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, FLinearColor> VectorParameters;

	/** Map of Texture Material Parameter Name to its Value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, UTexture*> TextureParameters;

protected:
	/** Check if the parameter of the specified type and name exists on material */
	static bool CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance, FName ParameterName,
												 EMaterialParameterType ParameterType);
	
	/** Update child material parameters */
	void ApplyMaterialChanges(USceneComponent* ChildComponent);

	/** Save this customization to CustomizationMaterialAsset */
	void SaveToDataAsset() const;
	
	/** ApplyMaterialChanges to child on child attached */
	virtual void OnChildAttached(USceneComponent* ChildComponent) override;

	/** SaveToDataAsset on register */
	virtual void OnRegister() override;
public:
	// Sets default values for this component's properties
	UCustomizationMaterialNameSpace();
};
