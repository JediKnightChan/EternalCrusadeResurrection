// Copyleft: All rights reversed


#include "Customization/CustomizationMaterialNameSpace.h"

#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "Components/MeshComponent.h"
#include "Customization/CustomizationElementaryModule.h"
#include "Materials/MaterialInstanceDynamic.h"


// Sets default values for this component's properties
UCustomizationMaterialNameSpace::UCustomizationMaterialNameSpace()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


FCustomizationMaterialNamespaceData UCustomizationMaterialNameSpace::GetMaterialCustomizationData(
	const FName NamespaceOverride) const
{
	const UCustomizationSavingNameSpace* SavingNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		UCustomizationSavingNameSpace>(this);

	// No UCustomizationSavingNameSpace among parents
	if (!SavingNamespace)
	{
		return {};
	}

	FName MyName = NamespaceOverride;
	if (NamespaceOverride.IsNone())
	{
		MyName = FName{UCustomizationUtilsLibrary::GetDisplayNameEnd(this)};
	}

	if (SavingNamespace->MaterialCustomizationData.Contains(MyName) && SavingNamespace->AllowedMaterialNamespaces.Contains(MyName))
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
		FName ChildCustomizationNamespaceOverride = NAME_None;
		TMap<FName, FName> SlotNamesCustomizationNamespacesOverride = {};

		if (UCustomizationElementaryModule* CustomizationElementaryModule = Cast<UCustomizationElementaryModule>(
			ChildComponent))
		{
			FName SupposedOverride = CustomizationElementaryModule->GetCustomizationNamespaceOverride();
			if (!SupposedOverride.IsNone())
			{
				ChildCustomizationNamespaceOverride = SupposedOverride;
			}

			SlotNamesCustomizationNamespacesOverride = CustomizationElementaryModule->GetSlotNamesNamespacesOverride();
		}


		if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(ChildComponent))
		{
			TArray<FName> SlotNames = MeshComponent->GetMaterialSlotNames();
			for (auto SlotName : SlotNames)
			{
				FName MaterialNamespace = ChildCustomizationNamespaceOverride;
				if (SlotNamesCustomizationNamespacesOverride.Contains(SlotName))
				{
					MaterialNamespace = SlotNamesCustomizationNamespacesOverride[SlotName];
				}

				const FCustomizationMaterialNamespaceData CustomizationData = GetMaterialCustomizationData(
					MaterialNamespace);
				ApplyMaterialChanges(MeshComponent, CustomizationData.ScalarParameters,
				                     CustomizationData.VectorParameters,
				                     CustomizationData.TextureParameters, {SlotName});
			}
		}
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
				if (SkinnedMeshComponent->GetSkinnedAsset())
				{
					TArray<FSkeletalMaterial> SkeletalMaterials = SkinnedMeshComponent->GetSkinnedAsset()->
						GetMaterials();
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

void UCustomizationMaterialNameSpace::SaveCMA()
{
	const UCustomizationSavingNameSpace* SavingNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		UCustomizationSavingNameSpace>(this);

	if (SavingNamespace)
	{
		FName MyName = FName{UCustomizationUtilsLibrary::GetDisplayNameEnd(this)};

		if (SavingNamespace->MaterialCustomizationData.Contains(MyName))
		{
			SavingNamespace->SaveCertainMaterialCustomizationData(MyName, SavingNamespace->MaterialCustomizationData[MyName], true);
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("CustomizationMaterialNamespace %s not found in parent CustomizationSavingNamespace"), *(GetNameSafe(this)));
		}
	} else
	{
		UE_LOG(LogTemp, Error, TEXT("CustomizationMaterialNamespace %s doesn't have parent of type CustomizationSavingNamespace"), *(GetNameSafe(this)))
	}
}
