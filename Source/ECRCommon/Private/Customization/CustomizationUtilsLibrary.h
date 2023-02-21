// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "CustomizationUtilsLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Join root dir and relative filepath, handling case when root dir does not end with slash */
	FORCEINLINE static FString GetFullSavePath(const FString SaveDestinationDirectory,
	                                           const FString SaveDestinationFilename)
	{
		FString Combined = FPaths::Combine(SaveDestinationDirectory, SaveDestinationFilename);
		FPaths::RemoveDuplicateSlashes(Combined);
		return Combined;
	}

	/** Get filename from relative path */
	FORCEINLINE static FString GetFilenameFromRelativePath(const FString RelativePath)
	{
		FString PathPart, FilenamePart, ExtensionPart;
		FPaths::Split(RelativePath, PathPart, FilenamePart, ExtensionPart);
		return FilenamePart;
	}

	/** Among parent components of given component, find first with type ComponentType */
	template <class ComponentType>
	FORCEINLINE static ComponentType* GetFirstParentComponentOfType(const USceneComponent* BaseComponent,
	                                                                const bool AllowFirstOnly = false)
	{
		ComponentType* FirstParentSkeletalMeshComponent = nullptr;

		// Getting all parent components
		TArray<USceneComponent*> Parents;
		BaseComponent->GetParentComponents(Parents);

		// Looping through parents to find first skeletal mesh
		for (USceneComponent* ParentComponent : Parents)
		{
			if (const TObjectPtr<ComponentType> ParentSkeletalMeshComponent = Cast<
				ComponentType>(ParentComponent))
			{
				FirstParentSkeletalMeshComponent = ParentSkeletalMeshComponent;
				break;
			}

			if (AllowFirstOnly)
			{
				break;
			}
		}

		return FirstParentSkeletalMeshComponent;
	}


	/** Get display name of component like in the BlueprintEditor */
	FORCEINLINE static FString GetDisplayNameEnd(const USceneComponent* Component)
	{
		FString DisplayName{""};
		if (Component)
		{
			DisplayName = UKismetSystemLibrary::GetDisplayName(Component);
		}

		// Remove blueprint name, separate by ".", get second
		FString FirstNameStart{""};
		FString FirstNameEnd = DisplayName;
		DisplayName.Split(".", &FirstNameStart, &FirstNameEnd);

		// Remove additional data, separate by " ", get first
		FString SecondNameStart = FirstNameEnd;
		FString SecondNameEnd{""};
		FirstNameEnd.Split(" ", &SecondNameStart, &SecondNameEnd);

		return SecondNameStart;
	}


	/** Get display name of component like in the BlueprintEditor of the first parent component with type ComponentType */
	template <class ComponentType>
	FORCEINLINE static FString GetFirstParentComponentOfTypeDisplayNameEnd(
		const USceneComponent* BaseComponent, const bool AllowFirstOnly = false)
	{
		const ComponentType* FirstParentComponentOfType = GetFirstParentComponentOfType<ComponentType>(
			BaseComponent, AllowFirstOnly);
		return GetDisplayNameEnd(FirstParentComponentOfType);
	}

	/** Among given array of Skeletal Materials, get indices of materials with given MaterialSlotName */
	FORCEINLINE static TArray<int32> GetMaterialIndices(TArray<FSkeletalMaterial>& SkeletalMeshMaterials,
	                                                    const FName MaterialSlotName)
	{
		TArray<int32> MaterialIndices;
		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMeshMaterials.Num(); ++MaterialIndex)
		{
			const FSkeletalMaterial& SkeletalMaterial = SkeletalMeshMaterials[MaterialIndex];
			if (SkeletalMaterial.MaterialSlotName == MaterialSlotName)
			{
				MaterialIndices.Add(MaterialIndex);
			}
		}
		return MaterialIndices;
	}

	/** Among given array of Static Materials, get indices of materials with given MaterialSlotName */
	FORCEINLINE static TArray<int32> GetMaterialIndices(TArray<FStaticMaterial>& StaticMaterials,
	                                                    const FName MaterialSlotName)
	{
		TArray<int32> MaterialIndices;
		for (int32 MaterialIndex = 0; MaterialIndex < StaticMaterials.Num(); ++MaterialIndex)
		{
			const FStaticMaterial& StaticMaterial = StaticMaterials[MaterialIndex];
			if (StaticMaterial.MaterialSlotName == MaterialSlotName)
			{
				MaterialIndices.Add(MaterialIndex);
			}
		}
		return MaterialIndices;
	}


	/** Get child (attachment) material customization namespace: split by '_' and return second part */
	FORCEINLINE static FString GetAttachmentMaterialCustomizationNamespace(const USceneComponent* ChildComponent)
	{
		const FString ChildComponentName = UCustomizationUtilsLibrary::GetDisplayNameEnd(ChildComponent);
		FString AttachmentName, AttachmentMaterialNamespace;

		// No CustomizationMaterialNamespace as no name part after '_'
		if (!ChildComponentName.Split("_", &AttachmentName, &AttachmentMaterialNamespace))
		{
			return "";
		}
		return AttachmentMaterialNamespace;
	};
};
