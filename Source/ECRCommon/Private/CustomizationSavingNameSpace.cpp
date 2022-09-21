// Copyleft: All rights reversed


#include "CustomizationSavingNameSpace.h"

#include "CustomizationElementaryModule.h"
#include "CustomizationLoaderAsset.h"
#include "CustomizationMaterialNameSpace.h"
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

	// Saving child component if customization component that can be saved
	TArray<UCustomizationElementaryAsset*> Assets;
	for (USceneComponent* Child : AllChildren)
	{
		// Saving if it's CustomizationElementaryModule
		if (const UCustomizationElementaryModule* ChildCustomizationElementaryModule = Cast<
			UCustomizationElementaryModule>(Child))
		{
			if (UCustomizationElementaryAsset* Asset = ChildCustomizationElementaryModule->SaveToDataAsset(bDoOverwrite))
			{
				Assets.AddUnique(Asset);
			}
			
		}

		// Saving if it's CustomizationElementaryModule
		if (const UCustomizationMaterialNameSpace* ChildCustomizationMaterialNameSpace = Cast<
			UCustomizationMaterialNameSpace>(Child))
		{
			ChildCustomizationMaterialNameSpace->SaveToDataAsset(bDoOverwrite);
		}
	}

	const FString SaveDestinationFilename = "CLA_" + UCustomizationUtilsLibrary::GetDisplayNameEnd(this);
	const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(
		SaveDestinationRootDirectory, "CLA/" + SaveDestinationFilename);
	

	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationLoaderAsset* LoadoutSave = NewObject<UCustomizationLoaderAsset>(
		NewPackage, *SaveDestinationFilename,
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
