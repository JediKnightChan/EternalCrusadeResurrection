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
#include "Particles/ParticleSystemComponent.h"
#include "UObject/SavePackage.h"


UCustomizationElementaryModule::UCustomizationElementaryModule()
{
	bInheritAnimations = true;
	MeshMergerNamespace = "";
	CustomizationNamespaceOverride = "";
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
	const FName AttachmentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(MyDisplayName);
	const FCustomizationMaterialNamespaceData MaterialData = GetMaterialCustomizationData(
		AttachmentMaterialNamespace);

	// Apply material customization data to child
	UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData.ScalarParameters,
	                                                      MaterialData.VectorParameters,
	                                                      MaterialData.TextureParameters, {});
}


FCustomizationMaterialNamespaceData UCustomizationElementaryModule::GetMaterialCustomizationData(
	const FName MaterialNamespace) const
{
	// No namespace specified
	if (MaterialNamespace.IsNone())
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
	SetLeaderPoseComponent(FirstParentSkeletalMeshComponent);
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
	if (GetSkeletalMeshAsset() == nullptr)
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
	FName ModuleTypeName = FName{ComponentName};

	if (!CustomizationSavingNameSpace->AllowedModuleNames.Contains(ModuleTypeName))
	{
		UE_LOG(LogTemp, Error, TEXT("Module name %s is not allowed, set it in AllowedModuleNames in SavingNamespace"),
		       *(ComponentName))
	}


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

	const FString SaveDestinationFilename = "CEA_" + ModuleTypeName.ToString() + "_" + ModuleCustomizationName;
	const FString SaveRelativePath = "Modules/" + ModuleTypeName.ToString() + "/" + SaveDestinationFilename;

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
	DataAssetSave->ModuleName = ModuleTypeName;

	// Setting base mesh
	DataAssetSave->BaseSkeletalMesh = GetSkeletalMeshAsset();

	// Setting merging namespace
	DataAssetSave->MeshMergeNamespace = MeshMergerNamespace;

	// Setting attach socket name
	FName ParentAttachSocketName = GetAttachSocketName();
	if (!ParentAttachSocketName.IsNone())
	{
		if (DataAssetSave->MeshMergeNamespace.IsNone())
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

	FName MaterialCustomizationNamespace = NAME_None;

	// Setting material customization namespace
	if (!CustomizationNamespaceOverride.IsNone())
	{
		MaterialCustomizationNamespace = CustomizationNamespaceOverride;
	}
	else
	{
		MaterialCustomizationNamespace = FName{
			UCustomizationUtilsLibrary::GetFirstParentComponentOfTypeDisplayNameEnd
			<UCustomizationMaterialNameSpace>(this)
		};
	}

	if (CustomizationSavingNameSpace->AllowedMaterialNamespaces.Contains(MaterialCustomizationNamespace))
	{
		DataAssetSave->MaterialCustomizationNamespace = MaterialCustomizationNamespace;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CMA namespace %s on component %s not allowed, will set to None"),
		       *(MaterialCustomizationNamespace.ToString()), *(GetNameSafe(this)))
	}

	// Setting material customization slot names
	DataAssetSave->MaterialCustomizationSlotNames = GetMaterialSlotNames();

	// Saving namespace overrides for slot names
	DataAssetSave->SlotNamesToMaterialNamespaceOverrides = SlotNamesNamespacesOverride;

	// Processing children
	for (TObjectPtr<USceneComponent> ChildComponent : AllChildren)
	{
		FString ChildName = UCustomizationUtilsLibrary::GetDisplayNameEnd(ChildComponent);
		if (AttachmentsForCAA.Contains(FName{ChildName}))
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping attachment %s for saving in CEA"), *(ChildName))
			continue;
		}

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
			FName ChildStaticMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildStaticMeshComponentName);

			FName SocketName = ChildStaticMeshComponent->GetAttachSocketName();

			if (!CustomizationSavingNameSpace->AllowedMaterialNamespaces.Contains(
				ChildStaticMeshComponentMaterialNamespace))
			{
				UE_LOG(LogTemp, Error, TEXT("Not allowed namespace %s on component %s"),
				       *(ChildStaticMeshComponentMaterialNamespace.ToString()),
				       *(GetNameSafe(ChildStaticMeshComponent)))
				ChildStaticMeshComponentMaterialNamespace = "";
			}

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
			if (!ChildSkeletalMeshComponent->GetSkeletalMeshAsset())
			{
				continue;
			}

			// Getting material namespace for child
			FString ChildSkeletalMeshComponentName =
				UCustomizationUtilsLibrary::GetDisplayNameEnd(
					ChildSkeletalMeshComponent);
			FName ChildSkeletalMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildSkeletalMeshComponentName);

			if (!CustomizationSavingNameSpace->AllowedMaterialNamespaces.Contains(
				ChildSkeletalMeshComponentMaterialNamespace))
			{
				UE_LOG(LogTemp, Error, TEXT("Not allowed namespace %s on component %s"),
				       *(ChildSkeletalMeshComponentMaterialNamespace.ToString()),
				       *(GetNameSafe(ChildStaticMeshComponent)))
				ChildSkeletalMeshComponentMaterialNamespace = "";
			}

			FName SocketName = ChildSkeletalMeshComponent->GetAttachSocketName();

			FCustomizationElementarySubmoduleSkeletal SkeletalData{
				ChildSkeletalMeshComponent->GetSkeletalMeshAsset(), SocketName,
				ChildSkeletalMeshComponent->GetRelativeTransform(), ChildSkeletalMeshComponentMaterialNamespace
			};
			DataAssetSave->SkeletalAttachments.Add(SkeletalData);
		}

		// Checking if it's particle system
		UParticleSystemComponent* ChildParticleSystemComponent = Cast<
			UParticleSystemComponent>(ChildComponent);
		if (ChildParticleSystemComponent != nullptr)
		{
			// If component present, but empty, skip
			if (!ChildParticleSystemComponent->Template)
			{
				continue;
			}

			FName SocketName = ChildParticleSystemComponent->GetAttachSocketName();

			FCustomizationElementarySubmoduleParticle ParticleData{
				ChildParticleSystemComponent->Template, SocketName,
				ChildParticleSystemComponent->GetRelativeTransform()
			};
			DataAssetSave->ParticleAttachments.Add(ParticleData);
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

UCustomizationAttachmentAsset* UCustomizationElementaryModule::SaveAttachmentsToDataAsset() const
{
	// Getting all children components
	TArray<TObjectPtr<USceneComponent>> AllChildren;
	GetChildrenComponents(true, AllChildren);

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

	if (CustomizationAttachmentAssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("ElementaryCustomizableModule %s has no declared"
			       " CustomizationAttachmentAssetName and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return nullptr;
	}

	const FString SaveRootDir = CustomizationSavingNameSpace->SaveDestinationRootDirectory;

	const FString ComponentName = UCustomizationUtilsLibrary::GetDisplayNameEnd(this);
	FName ModuleTypeName = FName{ComponentName};

	if (!CustomizationSavingNameSpace->AllowedModuleNames.Contains(ModuleTypeName))
	{
		UE_LOG(LogTemp, Error, TEXT("Module name %s is not allowed, set it in AllowedModuleNames in SavingNamespace"),
		       *(ComponentName))
	}

	FString ModuleCustomizationName = CustomizationAttachmentAssetName;

	const FString SaveDestinationFilename = "CAA_" + ModuleTypeName.ToString() + "_" + ModuleCustomizationName;
	const FString SaveRelativePath = "CAA/" + ModuleTypeName.ToString() + "/" + SaveDestinationFilename;

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

	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationAttachmentAsset* DataAssetSave = NewObject<UCustomizationAttachmentAsset>(
		NewPackage, *SaveDestinationFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);

	DataAssetSave->ModuleName = ModuleTypeName;

	// Processing children
	for (TObjectPtr<USceneComponent> ChildComponent : AllChildren)
	{
		FString ChildName = UCustomizationUtilsLibrary::GetDisplayNameEnd(ChildComponent);

		if (!AttachmentsForCAA.Contains(FName{ChildName}))
		{
			UE_LOG(LogTemp, Warning, TEXT("Attachment %s is not included in AttachmentsToSkipInCEA and won't be saved in CAA"), *(ChildName))
			continue;
		}

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

			FName ChildStaticMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildName);

			FName SocketName = ChildStaticMeshComponent->GetAttachSocketName();

			if (!CustomizationSavingNameSpace->AllowedMaterialNamespaces.Contains(
				ChildStaticMeshComponentMaterialNamespace))
			{
				UE_LOG(LogTemp, Error, TEXT("Not allowed namespace %s on component %s"),
				       *(ChildStaticMeshComponentMaterialNamespace.ToString()),
				       *(GetNameSafe(ChildStaticMeshComponent)))
				ChildStaticMeshComponentMaterialNamespace = "";
			}

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
			if (!ChildSkeletalMeshComponent->GetSkeletalMeshAsset())
			{
				continue;
			}

			FName ChildSkeletalMeshComponentMaterialNamespace = AttachmentsToMaterialNamespaces.FindRef(
				ChildName);

			FName SocketName = ChildSkeletalMeshComponent->GetAttachSocketName();

			if (!CustomizationSavingNameSpace->AllowedMaterialNamespaces.Contains(
				ChildSkeletalMeshComponentMaterialNamespace))
			{
				UE_LOG(LogTemp, Error, TEXT("Not allowed namespace %s on component %s"),
				       *(ChildSkeletalMeshComponentMaterialNamespace.ToString()),
				       *(GetNameSafe(ChildStaticMeshComponent)))
				ChildSkeletalMeshComponentMaterialNamespace = "";
			}

			FCustomizationElementarySubmoduleSkeletal SkeletalData{
				ChildSkeletalMeshComponent->GetSkeletalMeshAsset(), SocketName,
				ChildSkeletalMeshComponent->GetRelativeTransform(), ChildSkeletalMeshComponentMaterialNamespace
			};
			DataAssetSave->SkeletalAttachments.Add(SkeletalData);
		}

		// Checking if it's particle system
		UParticleSystemComponent* ChildParticleSystemComponent = Cast<
			UParticleSystemComponent>(ChildComponent);
		if (ChildParticleSystemComponent != nullptr)
		{
			// If component present, but empty, skip
			if (!ChildParticleSystemComponent->Template)
			{
				continue;
			}

			FName SocketName = ChildParticleSystemComponent->GetAttachSocketName();

			FCustomizationElementarySubmoduleParticle ParticleData{
				ChildParticleSystemComponent->Template, SocketName,
				ChildParticleSystemComponent->GetRelativeTransform()
			};
			DataAssetSave->ParticleAttachments.Add(ParticleData);
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

void UCustomizationElementaryModule::SaveToAttachmentAsset() const
{
	UCustomizationAttachmentAsset* AttachmentsToDataAsset = SaveAttachmentsToDataAsset();
}
