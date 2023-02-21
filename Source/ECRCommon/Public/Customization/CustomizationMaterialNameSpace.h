// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "CustomizationSavingNameSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "CustomizationMaterialNameSpace.generated.h"


UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationMaterialNameSpace : public USceneComponent
{
	GENERATED_BODY()

protected:
	/** Check if the parameter of the specified type and name exists on material */
	static bool CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance, FName ParameterName,
	                                             EMaterialParameterType ParameterType);

	/** ApplyMaterialChanges to child on child attached */
	virtual void OnChildAttached(USceneComponent* ChildComponent) override;

public:
	// Sets default values for this component's properties
	UCustomizationMaterialNameSpace();

	FCustomizationMaterialNamespaceData GetMaterialCustomizationData() const;

	/** Update child material parameters, pass non empty SlotNames array to limit material names that will be modified */
	void static ApplyMaterialChanges(USceneComponent* ChildComponent, const TMap<FName, float>& GivenScalarParameters,
	                                 const TMap<FName, FLinearColor>& GivenVectorParameters,
	                                 const TMap<FName, UTexture*>& GivenTextureParameters,
	                                 const TArray<FName> SlotNames);
};
