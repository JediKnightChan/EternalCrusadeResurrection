// Copyleft: All rights reversed


#include "Customization/CustomizationElementaryModule.h"

#include "Customization/CustomizationMaterialNameSpace.h"
#include "Customization/CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "Customization/CustomizationElementaryAsset.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SavePackage.h"


UCustomizationElementaryModule::UCustomizationElementaryModule()
{
	bInheritAnimations = true;
	MeshMergerNamespace = "";
}


void UCustomizationElementaryModule::OnRegister()
{
	Super::OnRegister();

	if (IsGarbageCollecting())
	{
		return;
	}

	InheritAnimationsIfNeeded();
}

void UCustomizationElementaryModule::OnChildAttached(USceneComponent* ChildComponent)
{
	Super::OnChildAttached(ChildComponent);

	if (IsGarbageCollecting())
	{
		return;
	}

	// Get material customization data for child
	const FString MyDisplayName = UCustomizationUtilsLibrary::GetDisplayNameEnd(ChildComponent);
	const FString AttachmentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(MyDisplayName);
	const FCustomizationMaterialNamespaceData MaterialData = GetMaterialCustomizationData(
		AttachmentMaterialNamespace);

	// Apply material customization data to child
	UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData.ScalarParameters,
	                                                      MaterialData.VectorParameters,
	                                                      MaterialData.TextureParameters, {});
}


FCustomizationMaterialNamespaceData UCustomizationElementaryModule::GetMaterialCustomizationData(
	const FString MaterialNamespace) const
{
	// No namespace specified
	if (MaterialNamespace == "")
	{
		return {};
	}

	const UCustomizationSavingNameSpace* SavingNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		UCustomizationSavingNameSpace>(this);

	// No CustomizationSavingNamespace in parents
	if (!SavingNamespace)
	{
		return {};
	}

	if (SavingNamespace->MaterialCustomizationData.Contains(MaterialNamespace))
	{
		return SavingNamespace->MaterialCustomizationData[MaterialNamespace];
	}

	// No data about the specified namespace
	return {};
}


void UCustomizationElementaryModule::InheritAnimationsIfNeeded()
{
	USkeletalMeshComponent* FirstParentSkeletalMeshComponent = nullptr;

	// Find first parent skeletal mesh, as we want to inherit animations from it
	if (bInheritAnimations)
	{
		FirstParentSkeletalMeshComponent = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
			USkeletalMeshComponent>(this);
	}

	// Inheriting animations or resetting them
	SetMasterPoseComponent(FirstParentSkeletalMeshComponent);
}


FString UCustomizationElementaryModule::GetFirstMaterialNameSpaceRaw(const USceneComponent* GivenComponent) const
{
	FString ChildStaticMeshComponentMaterialNamespace =
		UCustomizationUtilsLibrary::GetFirstParentComponentOfTypeDisplayNameEnd<
			UCustomizationMaterialNameSpace>(GivenComponent, true);
	return ChildStaticMeshComponentMaterialNamespace;
}


UCustomizationElementaryAsset* UCustomizationElementaryModule::SaveToDataAsset(bool bDoOverwrite) const
{
	// Getting all children components
	TArray<TObjectPtr<USceneComponent>> AllChildren;
	GetChildrenComponents(true, AllChildren);

	// Return if no skeletal mesh
	if (SkeletalMesh == nullptr)
	{
		return nullptr;
	}

	// Return if any of parents is same class (ElementaryCustomizableModule)
	if (UCustomizationUtilsLibrary::GetFirstParentComponentOfType<UCustomizationElementaryModule>(this) != nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ElementaryCustomizableModule %s has parent of same class and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return nullptr;
	}

	// Get saving namespace, return if none
	const UCustomizationSavingNameSpace* CustomizationSavingNameSpace =
		UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
			UCustomizationSavingNameSpace>(this);
	if (CustomizationSavingNameSpace == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("ElementaryCustomizableModule %s has no parent of class"
			       " CustomizationSavingNameSpace and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return nullptr;
	}

	const FString SaveRootDir = CustomizationSavingNameSpace->SaveDestinationRootDirectory;

	const FString ComponentName = UCustomizationUtilsLibrary::GetDisplayNameEnd(this);

	FString ModuleTypeName = ComponentName;
	FString ModuleCustomizationName;
	if (CustomizationSavingNameSpace->ModuleNamingMapping.Contains(ModuleTypeName))
	{
		ModuleCustomizationName = CustomizationSavingNameSpace->ModuleNamingMapping.FindRef(ModuleTypeName);
	}
	else if (!CustomizationSavingNameSpace->CommonModuleNaming.IsEmpty())
	{
		ModuleCustomizationName = CustomizationSavingNameSpace->CommonModuleNaming;
	}
	else
	{
		ModuleCustomizationName = UCustomizationUtilsLibrary::GetDisplayNameEnd(CustomizationSavingNameSpace);
	}

	const FString SaveDestinationFilename = "CEA_" + ModuleCustomizationName + "_" + ModuleTypeName;
	const FString SaveRelativePath = "Modules/" + ModuleTypeName + "/" + SaveDestinationFilename;

	const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(
		SaveRootDir, SaveRelativePath);

	// Return if invalid package path
	if (!FPackageName::IsValidLongPackageName(SaveDestinationPackagePath))
	{
		UE_LOG(LogTemp, Error,
		       TEXT("SaveDestinationRootDirectory of CustomizationSavingNameSpace and display names of elementary"
			       " module and CustomizationSavingNameSpace give invalid save package path: %s or"
			       " one of them is empty string"), *(SaveDestinationPackagePath));
		return nullptr;
	}

	// Return if overwriting disabled, but package already exists
	if (!bDoOverwrite && FPackageName::DoesPackageExist(SaveDestinationPackagePath))
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Package %s already exists and overwriting not requested, not saving"),
		       *(SaveDestinationPackagePath));

		FPackagePath MyPackagePath;
		FPackagePath::TryFromPackageName(FName{SaveDestinationPackagePath}, MyPackagePath);
		UPackage* Package = LoadPackage(nullptr, MyPackagePath, ELoadFlags::LOAD_EditorOnly);
		UCustomizationElementaryAsset* CustomizationElementaryAsset = FindObjectFast<UCustomizationElementaryAsset>(
			Package, FName{SaveDestinationFilename});
		return CustomizationElementaryAsset;
	}

	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationElementaryAsset* DataAssetSave = NewObject<UCustomizationElementaryAsset>(
		NewPackage, *SaveDestinationFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);

	// Setting name
	DataAssetSave->ModuleName = ComponentName;

	// Setting base mesh
	DataAssetSave->BaseSkeletalMesh = SkeletalMesh;

	// Setting merging namespace
	DataAssetSave->MeshMergeNamespace = MeshMergerNamespace;

	// Setting attach socket name
	FName ParentAttachSocketName = GetAttachSocketName();
	if (ParentAttachSocketName != NAME_None)
	{
		if (DataAssetSave->MeshMergeNamespace == "")
		{
			DataAssetSave->ParentAttachName = ParentAttachSocketName;
		}
		else
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("CustomizationElementaryAsset %s has set CustomizationMeshMergerNamepace,"
				       " attach socket name will be ignored"), *(UKismetSystemLibrary::GetDisplayName(this)));
		}
	}

	// Setting material customization namespace
	FString MaterialCustomizationNamespace = UCustomizationUtilsLibrary::GetFirstParentComponentOfTypeDisplayNameEnd<
		UCustomizationMaterialNameSpace>(this);
	DataAssetSave->MaterialCustomizationNamespace = MaterialCustomizationNamespace;

	// Setting material customization slot names
	DataAssetSave->MaterialCustomizationSlotNames = GetMaterialSlotNames();

	// Processing children
	for (TObjectPtr<USceneComponent> ChildComponent : AllChildren)
	{
		// Checking if it's static mesh
		UStaticMeshComponent* ChildStaticMeshComponent = Cast<
			UStaticMeshComponent>(ChildComponent);
		if (ChildStaticMeshComponent != nullptr)
		{
			// If component present, but empty, skip
			if (!ChildStaticMeshComponent->GetStaticMesh())
			{
				continue;
			}

			// Getting material namespace for child
			FString ChildStaticMeshComponentName =
				UCustomizationUtilsLibrary::GetDisplayNameEnd(ChildStaticMeshComponent);
			FString ChildStaticMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildStaticMeshComponentName);
			if (GetMaterialCustomizationData(ChildStaticMeshComponentMaterialNamespace).IsEmpty())
			{
				if (!ChildStaticMeshComponentName.IsEmpty())
				{
					UE_LOG(LogTemp, Warning,
					       TEXT(
						       "Material namespace of component %s isn't present in SavingNamespace or"
						       " doesn't contain any parameters, it will be set to None"
					       ), *(UKismetSystemLibrary::GetDisplayName(ChildStaticMeshComponent)))
					ChildStaticMeshComponentMaterialNamespace = "";
				}
			}

			FName SocketName = ChildStaticMeshComponent->GetAttachSocketName();
			SocketName = SocketName == NAME_None ? "" : SocketName;

			FCustomizationElementarySubmoduleStatic StaticData{
				ChildStaticMeshComponent->GetStaticMesh(), SocketName,
				ChildStaticMeshComponent->GetRelativeTransform(), ChildStaticMeshComponentMaterialNamespace
			};
			DataAssetSave->StaticAttachments.Add(StaticData);
		}

		// Checking if it's skeletal mesh
		USkeletalMeshComponent* ChildSkeletalMeshComponent = Cast<
			USkeletalMeshComponent>(ChildComponent);
		if (ChildSkeletalMeshComponent != nullptr)
		{
			// If component present, but empty, skip
			if (!ChildSkeletalMeshComponent->SkeletalMesh)
			{
				continue;
			}
			
			// Getting material namespace for child
			FString ChildSkeletalMeshComponentName =
				UCustomizationUtilsLibrary::GetDisplayNameEnd(
					ChildSkeletalMeshComponent);
			FString ChildSkeletalMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildSkeletalMeshComponentName);

			if (GetMaterialCustomizationData(ChildSkeletalMeshComponentMaterialNamespace).IsEmpty())
			{
				if (!ChildSkeletalMeshComponentMaterialNamespace.IsEmpty())
				{
					UE_LOG(LogTemp, Display,
					       TEXT(
						       "Material namespace of component %s isn't present in SavingNamespace or "
						       "doesn't contain any parameters, it will be set to None"
					       ), *(UKismetSystemLibrary::GetDisplayName(ChildSkeletalMeshComponent)))
				}

				ChildSkeletalMeshComponentMaterialNamespace = "";
			}

			FName SocketName = ChildSkeletalMeshComponent->GetAttachSocketName();
			SocketName = SocketName == NAME_None ? "" : SocketName;

			FCustomizationElementarySubmoduleSkeletal SkeletalData{
				ChildSkeletalMeshComponent->SkeletalMesh, SocketName,
				ChildSkeletalMeshComponent->GetRelativeTransform(), ChildSkeletalMeshComponentMaterialNamespace
			};
			DataAssetSave->SkeletalAttachments.Add(SkeletalData);
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

	return DataAssetSave;
}

void UCustomizationElementaryModule::SaveToDataAsset() const
{
	UCustomizationElementaryAsset* CustomizationElementaryAsset = SaveToDataAsset(true);
}
