// Copyleft: All rights reversed


#include "Customization/CustomizationAssetUtilities.h"

#include "ContentBrowserModule.h"
#include "ECRCommon/Public/Customization/CustomizationElementaryBundle.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SavePackage.h"


void UCustomizationAssetUtilities::CreateElementaryAssetBundle()
{
	TArray<FAssetData> OutSelectedAssets;
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(
		"ContentBrowser");
	IContentBrowserSingleton& ContentBrowserSingleton = ContentBrowserModule.Get();
	ContentBrowserSingleton.GetSelectedAssets(OutSelectedAssets);

	FString DirectoryName{""};
	FString LastDirectoryName{""};
	
	TMap<FString, UCustomizationElementaryAsset*> NameToElementaryAsset;
	for (FAssetData SelectedAsset : OutSelectedAssets)
	{
		if (DirectoryName == "")
		{
			FString PathPart, FilenamePart, ExtensionPart;
			FPaths::Split(SelectedAsset.PackagePath.ToString(), PathPart, FilenamePart, ExtensionPart);
			DirectoryName = PathPart + "/" + FilenamePart + "/";
			LastDirectoryName = FilenamePart;
		}
		
		if (SelectedAsset.AssetClass == UCustomizationElementaryAsset::StaticClass()->GetFName())
		{
			UObject* Object = SelectedAsset.GetAsset();
			if (UCustomizationElementaryAsset* CustomizationElementaryAsset = Cast<
				UCustomizationElementaryAsset>(Object))
			{
				NameToElementaryAsset.Add(
					UKismetSystemLibrary::GetDisplayName(CustomizationElementaryAsset), CustomizationElementaryAsset);
			}
		}
	}

	if (NameToElementaryAsset.Num() == 0)
	{
		return;
	}
	
	// Creating package for saving data
	const FString SaveDestinationFilename{LastDirectoryName + "Bundle"};
	const FString SaveDestinationPath = DirectoryName + SaveDestinationFilename;


	UPackage* NewPackage = CreatePackage(*SaveDestinationPath);
	UCustomizationElementaryBundle* CustomizationElementaryBundle = NewObject<UCustomizationElementaryBundle>(
		NewPackage, *SaveDestinationFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);

	CustomizationElementaryBundle->NameToElementaryAsset = NameToElementaryAsset;

	// Saving package
	FString const PackageName = NewPackage->GetName();
	FString const PackageFileName = FPackageName::LongPackageNameToFilename(
		PackageName, FPackageName::GetAssetPackageExtension());

	FAssetRegistryModule::AssetCreated(CustomizationElementaryBundle);

	FSavePackageArgs SavePackageArgs;
	SavePackageArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SavePackageArgs.SaveFlags = SAVE_NoError;
	UPackage::SavePackage(NewPackage, CustomizationElementaryBundle, *PackageFileName, SavePackageArgs);

}
