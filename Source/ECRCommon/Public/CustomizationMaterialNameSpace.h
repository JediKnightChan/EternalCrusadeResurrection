// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "CustomizationMaterialNameSpace.generated.h"


UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationMaterialNameSpace : public USceneComponent
{
	GENERATED_BODY()

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
	TMap<FName, UTexture*> TextureParameters;

protected:
	/** Get first parent of component */
	USceneComponent* GetFirstParent() const;

	/** Check if the parameter of the specified type and name exists on material */
	static bool CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance, FName ParameterName,
	                                             EMaterialParameterType ParameterType);

	/** ApplyMaterialChanges to child on child attached */
	virtual void OnChildAttached(USceneComponent* ChildComponent) override;

	/** Inherit socket behaviour from first parent */
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const override;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const override;
	virtual FVector GetSocketLocation(FName InSocketName) const override;
	virtual FRotator GetSocketRotation(FName InSocketName) const override;
	virtual FQuat GetSocketQuaternion(FName InSocketName) const override;
	virtual bool DoesSocketExist(FName InSocketName) const override;
	virtual bool HasAnySockets() const override;

public:
	// Sets default values for this component's properties
	UCustomizationMaterialNameSpace();

	/** Update child material parameters, pass non empty SlotNames array to limit material names that will be modified */
	void static ApplyMaterialChanges(USceneComponent* ChildComponent, const TMap<FName, float>& GivenScalarParameters,
	                                 const TMap<FName, FLinearColor>& GivenVectorParameters,
	                                 const TMap<FName, UTexture*>& GivenTextureParameters,
	                                 const TArray<FName> SlotNames);

	/** Save this customization to CustomizationMaterialAsset */
	void SaveToDataAsset(bool bDoOverwrite) const;

	/** Save this customization to CustomizationMaterialAsset, overwriting it if it already exists,
	 * callable in Editor */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveToDataAsset() const;
};
