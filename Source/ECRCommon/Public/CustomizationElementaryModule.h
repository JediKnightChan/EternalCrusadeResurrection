// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "CustomizationElementaryAsset.h"
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

protected:
	virtual void OnRegister() override;

	/* Inheriting animations from first SkeletalMeshComponent parent if requested */
	void InheritAnimationsIfNeeded();

	/** Get material namespace for given component */
	template <class Component>
	FString GetFirstMaterialNameSpaceRaw(Component* GivenComponent) const;

public:
	UCustomizationElementaryModule();

	/* Save this module into a CustomizationElementaryAsset */
	UCustomizationElementaryAsset* SaveToDataAsset(bool bDoOverwrite) const;

	/* Save this module into a CustomizationElementaryAsset, overwriting it if it already exists,
	 * callable in Editor */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveToDataAsset() const;
};
