// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "CustomizationElementaryAsset.h"
#include "CustomizationSavingNameSpace.h"
#include "Components/SkeletalMeshComponent.h"
#include "CustomizationElementaryModule.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(ModularCustomizationCommon), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationElementaryModule : public USkeletalMeshComponent
{
	GENERATED_BODY()

	/** Whether to inherit animations from first SkeletalMeshComponent parent */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bInheritAnimations;

	/** Modules with common non-empty MeshMergerNamespace will be merged as one skeletal mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString MeshMergerNamespace;

	/** Material namespaces for attachments */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FString, FString> AttachmentsToMaterialNamespaces;

	/** This will override material namespace (one that this module is attached to) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString CustomizationNamespaceOverride;

	/** This will override material namespace for concise slot names */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FName, FString> SlotNamesNamespacesOverride;
protected:
	/* Inheriting animations from first SkeletalMeshComponent parent if requested */
	void InheritAnimationsIfNeeded();

	/** Get material namespace for given component */
	FString GetFirstMaterialNameSpaceRaw(const USceneComponent* GivenComponent) const;

	/** Get material customization data for given material namespace */
	FCustomizationMaterialNamespaceData GetMaterialCustomizationData(FString MaterialNamespace) const;

	/** Inherit animations on register */
	virtual void OnRegister() override;

	/** Apply material changes on child attached */
	virtual void OnChildAttached(USceneComponent* ChildComponent) override;

public:
	UCustomizationElementaryModule();

	/* Save this module into a CustomizationElementaryAsset */
	UCustomizationElementaryAsset* SaveToDataAsset(bool bDoOverwrite) const;

	/* Save this module into a CustomizationElementaryAsset, overwriting it if it already exists,
	 * callable in Editor */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveToDataAsset() const;

	FORCEINLINE FString GetCustomizationNamespaceOverride()
	{
		return CustomizationNamespaceOverride;
	}

	FORCEINLINE TMap<FName, FString> GetSlotNamesNamespacesOverride()
	{
		return SlotNamesNamespacesOverride;
	}
};
