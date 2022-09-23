// Copyleft: All rights reversed


#include "Customization/CustomizationMaterialNameSpace.h"

#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values for this component's properties
UCustomizationMaterialNameSpace::UCustomizationMaterialNameSpace()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


FCustomizationMaterialNamespaceData UCustomizationMaterialNameSpace::GetMaterialCustomizationData() const
{
	const UCustomizationSavingNameSpace* SavingNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		UCustomizationSavingNameSpace>(this);

	// No UCustomizationSavingNameSpace among parents
	if (!SavingNamespace)
	{
		return {};
	}

	const FString MyName = UCustomizationUtilsLibrary::GetDisplayNameEnd(this);
	if (SavingNamespace->MaterialCustomizationData.Contains(MyName))
	{
		return SavingNamespace->MaterialCustomizationData[MyName];
	}

	// UCustomizationSavingNameSpace parent doesn't contain our namespace
	return {};
}


void UCustomizationMaterialNameSpace::OnChildAttached(USceneComponent* ChildComponent)
{
	Super::OnChildAttached(ChildComponent);

	if (!IsGarbageCollecting())
	{
		const FCustomizationMaterialNamespaceData CustomizationData = GetMaterialCustomizationData();
		ApplyMaterialChanges(ChildComponent, CustomizationData.ScalarParameters, CustomizationData.VectorParameters,
		                     CustomizationData.TextureParameters, {});
	}
}


bool UCustomizationMaterialNameSpace::CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance,
                                                                       const FName ParameterName,
                                                                       const EMaterialParameterType ParameterType)
{
	FMaterialParameterMetadata OutValue;
	const FHashedMaterialParameterInfo ParameterInfo{ParameterName};
	return MaterialInstance->GetParameterValue(ParameterType, ParameterInfo, OutValue);
}


void UCustomizationMaterialNameSpace::ApplyMaterialChanges(USceneComponent* ChildComponent,
                                                           const TMap<FName, float>& GivenScalarParameters,
                                                           const TMap<FName, FLinearColor>& GivenVectorParameters,
                                                           const TMap<FName, UTexture*>& GivenTextureParameters,
                                                           const TArray<FName> SlotNames)
{
	if (UMeshComponent* MeshChildComponent = Cast<UMeshComponent>(ChildComponent))
	{
		TArray<FName> MaterialNames = MeshChildComponent->GetMaterialSlotNames();

		const USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(MeshChildComponent);
		const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshChildComponent);

		for (const FName MaterialName : MaterialNames)
		{
			// Continue if SlotNames non empty and doesn't contain material name we got
			if (SlotNames.Num() > 0 && !SlotNames.Contains(MaterialName))
			{
				continue;
			}

			// Getting material indices corresponding to one material slot name
			TArray<int32> MaterialIndices = {};

			if (SkinnedMeshComponent)
			{
				if (SkinnedMeshComponent->SkeletalMesh)
				{
					TArray<FSkeletalMaterial> SkeletalMaterials = SkinnedMeshComponent->SkeletalMesh->GetMaterials();
					MaterialIndices = UCustomizationUtilsLibrary::GetMaterialIndices(SkeletalMaterials, MaterialName);
				}
			}
			else if (StaticMeshComponent)
			{
				if (StaticMeshComponent->GetStaticMesh())
				{
					TArray<FStaticMaterial> StaticMaterials = StaticMeshComponent->GetStaticMesh()->
						GetStaticMaterials();
					MaterialIndices = UCustomizationUtilsLibrary::GetMaterialIndices(StaticMaterials, MaterialName);
				}
			}
			else
			{
				// Not static or skeletal mesh component, can't get materials
				continue;
			}

			for (const int32 MaterialIndex : MaterialIndices)
			{
				UMaterialInterface* MaterialInterface = MeshChildComponent->GetMaterial(MaterialIndex);

				if (const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(MaterialInterface))
				{
					UMaterialInstanceDynamic* MaterialInstanceDynamic = MeshChildComponent->
						CreateDynamicMaterialInstance(
							MaterialIndex, MaterialInterface);

					for (const TTuple<FName, float> NameAndScalarValue : GivenScalarParameters)
					{
						if (CheckIfMaterialContainsParameter(MaterialInstance, NameAndScalarValue.Key,
						                                     EMaterialParameterType::Scalar))
						{
							MaterialInstanceDynamic->SetScalarParameterValue(
								NameAndScalarValue.Key, NameAndScalarValue.Value);
						}
					}

					for (const TTuple<FName, FLinearColor> NameAndVectorValue : GivenVectorParameters)
					{
						if (CheckIfMaterialContainsParameter(MaterialInstance, NameAndVectorValue.Key,
						                                     EMaterialParameterType::Vector))
						{
							MaterialInstanceDynamic->SetVectorParameterValue(
								NameAndVectorValue.Key, NameAndVectorValue.Value);
						}
					}

					for (const TTuple<FName, UTexture*> NameAndTextureValue : GivenTextureParameters)
					{
						if (CheckIfMaterialContainsParameter(MaterialInstance, NameAndTextureValue.Key,
						                                     EMaterialParameterType::Texture))
						{
							MaterialInstanceDynamic->SetTextureParameterValue(
								NameAndTextureValue.Key, NameAndTextureValue.Value);
						}
					}
				}
			}
		}
	}
}
