// Copyleft: All rights reversed


#include "Customization/CustomizationSavingNameSpace.h"

#include "Customization/CustomizationElementaryModule.h"
#include "Customization/CustomizationLoaderAsset.h"
#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationMaterialNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// Sets default values for this component's properties
UCustomizationSavingNameSpace::UCustomizationSavingNameSpace()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SaveDestinationRootDirectory = "";
	// ...
}


void UCustomizationSavingNameSpace::SaveLoadout(const bool bDoOverwrite)
{
	// Getting all children components
	TArray<USceneComponent*> AllChildren;
	GetChildrenComponents(true, AllChildren);

	// Saving customization elementary modules among children
	TArray<UCustomizationElementaryAsset*> Assets;
	for (USceneComponent* Child : AllChildren)
	{
		// Check if it's CustomizationElementaryModule
		if (const UCustomizationElementaryModule* ChildCustomizationElementaryModule = Cast<
			UCustomizationElementaryModule>(Child))
		{
			if (UCustomizationElementaryAsset* Asset = ChildCustomizationElementaryModule->
				SaveToDataAsset(bDoOverwrite))
			{
				Assets.AddUnique(Asset);
			}
		}
	}

	// Saving material customization assets
	SaveMaterialCustomizationData(bDoOverwrite);

	FString FinalFilename;
	if (!SaveDestinationFilename.IsEmpty())
	{
		FinalFilename = SaveDestinationFilename.Replace(TEXT("/"), TEXT("_")).Replace(TEXT("\\"), TEXT("_"));
	}
	else
	{
		// Don't save loadout
		return;
	}

	const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(
		SaveDestinationRootDirectory, "CLA/" + FinalFilename);


	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationLoaderAsset* LoadoutSave = NewObject<UCustomizationLoaderAsset>(
		NewPackage, *FinalFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);

	// Setting parameters
	LoadoutSave->ElementaryAssets = Assets;

	// Saving package
	FString const PackageName = NewPackage->GetName();
	FString const PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());

	// NewPackage->SetDirtyFlag(true);
	FAssetRegistryModule::AssetCreated(LoadoutSave);

	FSavePackageArgs SavePackageArgs;
	SavePackageArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SavePackageArgs.SaveFlags = SAVE_NoError;
	UPackage::SavePackage(NewPackage, LoadoutSave, *PackageFileName, SavePackageArgs);
}


void UCustomizationSavingNameSpace::SaveLoadoutOverwritingExistingModules()
{
	SaveLoadout(true);
}


void UCustomizationSavingNameSpace::SaveLoadoutSkippingExistingModules()
{
	SaveLoadout(false);
}


void UCustomizationSavingNameSpace::SaveMaterialCustomizationData(bool bDoOverwrite) const
{
	for (auto [Namespace, CustomizationData] : MaterialCustomizationData)
	{
		// Getting direct children components
		TArray<TObjectPtr<USceneComponent>> DirectChildren;
		GetChildrenComponents(false, DirectChildren);

		const FString DestinationFilename = UCustomizationUtilsLibrary::GetFilenameFromRelativePath(
			CustomizationData.RelativeSavePath);
		const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(
			SaveDestinationRootDirectory, CustomizationData.RelativeSavePath);

		// Return if invalid package path
		if (CustomizationData.RelativeSavePath == "" || !FPackageName::IsValidLongPackageName(
			SaveDestinationPackagePath))
		{
			UE_LOG(LogTemp, Error,
			       TEXT("SaveDestinationRootDirectory of CustomizationSavingNameSpace and relative path property of"
				       " materials namespace give invalid save package path: %s or "
				       "one of them is empty string"), *(SaveDestinationPackagePath));
			return;
		}

		// Return if overwriting disabled, but package already exists
		if (!bDoOverwrite && FPackageName::DoesPackageExist(SaveDestinationPackagePath))
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("Package %s already exists and overwriting not requested, not saving"),
			       *(SaveDestinationPackagePath));
			continue;;
		}

		// Creating package for saving data
		UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
		UCustomizationMaterialAsset* DataAssetSave = NewObject<UCustomizationMaterialAsset>(
			NewPackage, *DestinationFilename,
			EObjectFlags::RF_Public |
			EObjectFlags::RF_Standalone |
			RF_MarkAsRootSet);

		// Setting material namespace
		DataAssetSave->MaterialNamespace = Namespace;

		// Setting parameters
		DataAssetSave->ScalarParameters = CustomizationData.ScalarParameters;
		DataAssetSave->VectorParameters = CustomizationData.VectorParameters;
		DataAssetSave->TextureParameters = CustomizationData.TextureParameters;


		// Saving package
		FString const PackageName = NewPackage->GetName();
		FString const PackageFileName = FPackageName::LongPackageNameToFilename(
			PackageName, FPackageName::GetAssetPackageExtension());

		// NewPackage->SetDirtyFlag(true);
		FAssetRegistryModule::AssetCreated(DataAssetSave);

		FSavePackageArgs SavePackageArgs;
		SavePackageArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SavePackageArgs.SaveFlags = SAVE_NoError;
		UPackage::SavePackage(NewPackage, DataAssetSave, *PackageFileName, SavePackageArgs);
	}
}
