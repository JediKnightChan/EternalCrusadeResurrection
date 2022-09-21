// Copyleft: All rights reversed


#include "CustomizationMaterialNameSpace.h"

#include "CustomizationMaterialAsset.h"
#include "CustomizationSavingNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/MeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/SavePackage.h"


// Sets default values for this component's properties
UCustomizationMaterialNameSpace::UCustomizationMaterialNameSpace()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	RelativeSavePath = "";
}


void UCustomizationMaterialNameSpace::OnChildAttached(USceneComponent* ChildComponent)
{
	Super::OnChildAttached(ChildComponent);

	if (!IsGarbageCollecting())
	{
		ApplyMaterialChanges(ChildComponent, ScalarParameters, VectorParameters, TextureParameters, {});
	}
}


void UCustomizationMaterialNameSpace::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		ParentComponent->QuerySupportedSockets(OutSockets);
	}
}


FTransform UCustomizationMaterialNameSpace::GetSocketTransform(FName InSocketName,
                                                               ERelativeTransformSpace TransformSpace) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->GetSocketTransform(InSocketName, TransformSpace);
	}
	return FTransform{};
}


FVector UCustomizationMaterialNameSpace::GetSocketLocation(FName InSocketName) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->GetSocketLocation(InSocketName);
	}
	return FVector{};
}


FRotator UCustomizationMaterialNameSpace::GetSocketRotation(FName InSocketName) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->GetSocketRotation(InSocketName);
	}
	return FRotator{};
}


FQuat UCustomizationMaterialNameSpace::GetSocketQuaternion(FName InSocketName) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->GetSocketQuaternion(InSocketName);
	}
	return FQuat{};
}


bool UCustomizationMaterialNameSpace::DoesSocketExist(FName InSocketName) const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->DoesSocketExist(InSocketName);
	}
	return false;
}


bool UCustomizationMaterialNameSpace::HasAnySockets() const
{
	if (const USceneComponent* ParentComponent = GetFirstParent())
	{
		return ParentComponent->HasAnySockets();
	}
	return false;
}


bool UCustomizationMaterialNameSpace::CheckIfMaterialContainsParameter(const UMaterialInstance* MaterialInstance,
                                                                       const FName ParameterName,
                                                                       const EMaterialParameterType ParameterType)
{
	FMaterialParameterMetadata OutValue;
	const FHashedMaterialParameterInfo ParameterInfo{ParameterName};
	return MaterialInstance->GetParameterValue(ParameterType, ParameterInfo, OutValue);
}

USceneComponent* UCustomizationMaterialNameSpace::GetFirstParent() const
{
	TArray<USceneComponent*> Parents;
	GetParentComponents(Parents);
	UE_LOG(LogTemp, Warning, TEXT("Parent num is %d"), Parents.Num());
	if (Parents.Num() != 0)
	{
		return Parents[0];
	}
	UE_LOG(LogTemp, Warning, TEXT("Nullptr GFP"))
	return nullptr;
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

		for (const FName MaterialName : MaterialNames)
		{
			// Continue if SlotNames non empty and doesn't contain material name we got
			if (SlotNames.Num() > 0 && !SlotNames.Contains(MaterialName))
			{
				continue;
			}

			const int32 MaterialIndex = MeshChildComponent->GetMaterialIndex(MaterialName);
			UMaterialInterface* MaterialInterface = MeshChildComponent->GetMaterial(MaterialIndex);

			if (const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(MaterialInterface))
			{
				UMaterialInstanceDynamic* MaterialInstanceDynamic = MeshChildComponent->CreateDynamicMaterialInstance(
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


void UCustomizationMaterialNameSpace::SaveToDataAsset(bool bDoOverwrite) const
{
	// Getting direct children components
	TArray<TObjectPtr<USceneComponent>> DirectChildren;
	GetChildrenComponents(false, DirectChildren);

	// Return if children amount is 0 or no parameters
	if (DirectChildren.Num() == 0 || (ScalarParameters.Num() == 0 && VectorParameters.Num() == 0 && TextureParameters.
		Num() == 0))
	{
		return;
	}

	// Get saving namespace, return if none
	const UCustomizationSavingNameSpace* CustomizationSavingNameSpace =
		UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
			UCustomizationSavingNameSpace>(this);
	if (CustomizationSavingNameSpace == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("CustomizationMaterialNamespace %s has no parent of class"
			       " CustomizationSavingNameSpace and won't be saved"),
		       *(UKismetSystemLibrary::GetDisplayName(this)));
		return;
	}

	const FString SaveRootDir = CustomizationSavingNameSpace->SaveDestinationRootDirectory;
	const FString SaveDestinationFilename = UCustomizationUtilsLibrary::GetFilenameFromRelativePath(RelativeSavePath);
	const FString SaveDestinationPackagePath = UCustomizationUtilsLibrary::GetFullSavePath(
		SaveRootDir, RelativeSavePath);

	// Return if invalid package path
	if (RelativeSavePath == "" || !FPackageName::IsValidLongPackageName(SaveDestinationPackagePath))
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
		UE_LOG(LogTemp, Error,
		       TEXT("Package %s already exists and overwriting not requested, not saving"),
		       *(SaveDestinationPackagePath));
		return;
	}

	// Creating package for saving data
	UPackage* NewPackage = CreatePackage(*SaveDestinationPackagePath);
	UCustomizationMaterialAsset* DataAssetSave = NewObject<UCustomizationMaterialAsset>(
		NewPackage, *SaveDestinationFilename,
		EObjectFlags::RF_Public |
		EObjectFlags::RF_Standalone |
		RF_MarkAsRootSet);

	// Setting material namespace: left part before underscore
	DataAssetSave->MaterialNamespace = UCustomizationUtilsLibrary::GetMaterialNameSpaceReal(
		UCustomizationUtilsLibrary::GetDisplayNameEnd(this));

	// Setting parameters
	DataAssetSave->ScalarParameters = ScalarParameters;
	DataAssetSave->VectorParameters = VectorParameters;
	DataAssetSave->TextureParameters = TextureParameters;


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

void UCustomizationMaterialNameSpace::SaveToDataAsset() const
{
	SaveToDataAsset(true);
}
