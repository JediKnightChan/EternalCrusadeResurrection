#include "Customization/CustomizationMaterialLoaderComponent.h"
#include "Customization/CustomizationMaterialNameSpace.h"
#include "Customization/CustomizationUtilsLibrary.h"

UCustomizationMaterialLoaderComponent::UCustomizationMaterialLoaderComponent()
{
	bLoadOnBeginPlay = false;
}

void UCustomizationMaterialLoaderComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bLoadOnBeginPlay)
	{
		LoadMaterialCustomization({});
	}
}

void UCustomizationMaterialLoaderComponent::LoadMaterialCustomization(
	const TArray<UCustomizationMaterialAsset*> NewConfigs)
{
	if (!NewConfigs.IsEmpty())
	{
		MaterialConfigs = NewConfigs;
	}

	// Material namespace data
	TMap<FString, UCustomizationMaterialAsset*> MaterialNamespacesToData;
	for (UCustomizationMaterialAsset* Config : MaterialConfigs)
	{
		if (!Config)
		{
			UE_LOG(LogAssetData, Warning, TEXT("Material config is null on %s"), *(GetNameSafe(GetOwner())));
			continue;
		}

		MaterialNamespacesToData.Add(Config->MaterialNamespace, Config);
	}

	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);

	for (USceneComponent* ChildComponent : Children)
	{
		FString MaterialNamespace = UCustomizationUtilsLibrary::GetAttachmentMaterialCustomizationNamespace(
			ChildComponent);
		if (MaterialNamespacesToData.Contains(MaterialNamespace))
		{
			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[MaterialNamespace];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters, {});
		}
	}
}
