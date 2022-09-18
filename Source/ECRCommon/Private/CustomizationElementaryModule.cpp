// Copyleft: All rights reversed


#include "CustomizationElementaryModule.h"

#include "CustomizationMaterialNameSpace.h"
#include "CustomizationMeshMergerNameSpace.h"
#include "CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "CustomizationElementaryAsset.h"
#include "MaterialCustomizationComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SavePackage.h"


UCustomizationElementaryModule::UCustomizationElementaryModule()
{
	bWantsSave = true;
	bInheritAnimations = true;
	bAllowSaveWithoutChildren = true;
}


void UCustomizationElementaryModule::OnRegister()
{
	Super::OnRegister();

	if (IsGarbageCollecting())
	{
		UE_LOG(LogTemp, Warning, TEXT("Garbage collecting"))
		return;
	}

	if (IsCompiling())
	{
		UE_LOG(LogTemp, Warning, TEXT("Compiling"))
	}

	if (IsBeingDestroyed())
	{
		UE_LOG(LogTemp, Warning, TEXT("Destroyed"));
	}

	InheritAnimationsIfNeeded();
	SaveToDataAsset();
}


void UCustomizationElementaryModule::InheritAnimationsIfNeeded()
{
	USkeletalMeshComponent* FirstParentSkeletalMeshComponent = nullptr;

	// Find first parent skeletal mesh, as we want to inherit animations from it
	if (bInheritAnimations)
	{
		FirstParentSkeletalMeshComponent = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<USkeletalMeshComponent>(this);
	}

	// Inheriting animations or resetting them
	SetMasterPoseComponent(FirstParentSkeletalMeshComponent);
}


void UCustomizationElementaryModule::SaveToDataAsset() const
{
	// Getting all children components
	TArray<TObjectPtr<USceneComponent>> AllChildren;
	GetChildrenComponents(true, AllChildren);

	// Return if no skeletal mesh or children amount is 0 without permission or doesn't want to be saved
	if (SkeletalMesh == nullptr || (AllChildren.Num() == 0 && !bAllowSaveWithoutChildren) || !bWantsSave)
	{
		return;
	}

	// Return if any of parents is same class (ElementaryCustomizableModule)
	if (UCustomizationUtilsLibrary::GetFirstParentComponentOfType<UCustomizationElementaryModule>(this) != nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ElementaryCustomizableModule %s has parent of same class and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	// Get saving namespace, return if none
	const UCustomizationSavingNameSpace* CustomizationSavingNameSpace = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		UCustomizationSavingNameSpace>(this);
	if (CustomizationSavingNameSpace == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ElementaryCustomizableModule %s has no parent of class"
			       " CustomizationSavingNameSpace and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	const FString SaveRootDir = CustomizationSavingNameSpace->SaveDestinationRootDirectory;
	const FString SaveDestinationFilename = UCustomizationUtilsLibrary::GetDisplayNameEnd(CustomizationSavingNameSpace);
	const FString SaveRelativePath = UCustomizationUtilsLibrary::GetDisplayNameEnd(this) + "/" + SaveDestinationFilename;
	
	const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(SaveRootDir, SaveRelativePath);

	// Return if invalid package path
	if (!FPackageName::IsValidLongPackageName(SaveDestinationPackagePath))
	{
		UE_LOG(LogTemp, Error,
		       TEXT("SaveDestinationRootDirectory of CustomizationSavingNameSpace and display names of elementary"
			       " module and CustomizationSavingNameSpace give invalid save package path: %s or"
			       " one of them is empty string"), *(SaveDestinationPackagePath));
		return;
	}

	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationElementaryAsset* DataAssetSave = NewObject<UCustomizationElementaryAsset>(
		NewPackage, *SaveDestinationFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);
	
	// Setting base mesh
	DataAssetSave->BaseSkeletalMesh = SkeletalMesh;

	// Setting attach socket name
	DataAssetSave->ParentAttachName = GetAttachSocketName();

	// Setting mesh merge namespace
	DataAssetSave->MeshMergeNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfTypeDisplayNameEnd<
		UCustomizationMeshMergerNameSpace>(this);

	FString MaterialCustomizationNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfTypeDisplayNameEnd<
		UCustomizationMaterialNameSpace>(this);

	// Setting material customization namespace
	DataAssetSave->MaterialCustomizationNamespace = MaterialCustomizationNamespace;

	// Processing children
	for (TObjectPtr<USceneComponent> ChildComponent : AllChildren)
	{
		// Checking if it's static mesh
		const TObjectPtr<UStaticMeshComponent> ChildStaticMeshComponent = Cast<
			UStaticMeshComponent>(ChildComponent);
		if (ChildStaticMeshComponent != nullptr)
		{
			DataAssetSave->StaticMeshesToSocketNames.Add(ChildStaticMeshComponent->GetStaticMesh(),
			                                             ChildStaticMeshComponent->GetAttachSocketName());
		}

		// Checking if it's skeletal mesh
		const TObjectPtr<USkeletalMeshComponent> ChildSkeletalMeshComponent = Cast<
			USkeletalMeshComponent>(ChildComponent);
		if (ChildSkeletalMeshComponent != nullptr)
		{
			DataAssetSave->SkeletalMeshesToSocketNames.Add(ChildSkeletalMeshComponent->SkeletalMesh,
			                                               ChildSkeletalMeshComponent->GetAttachSocketName());
		}
	}

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
