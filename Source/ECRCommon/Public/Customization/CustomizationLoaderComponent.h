// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "CustomizationElementaryAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "CustomizationLoaderComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(ModularCustomizationCommon), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationLoaderComponent : public USceneComponent
{
	GENERATED_BODY()

	/** Whether to load customization on BeginPlay or load function will be called externally */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bLoadOnBeginPlay;

	/** Whether to inherit animation from the first SkeletalMeshComponent parent via Master Pose */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bInheritParentAnimations;

	/** Whether to use skeleton from the first SkeletalMeshComponent parent for mesh merging */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bUseParentSkeleton;

	/** Collision profile name to use for static and skeletal components. Leave None for default */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FName CollisionProfileName;

	/** Modular mesh customization config we want to load */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UCustomizationLoaderAsset* AssetConfig;

	/** Material customization configs we want to load */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<class UCustomizationMaterialAsset*> MaterialConfigs;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<USceneComponent*> SpawnedComponents;

protected:
	/** Spawn child component for Component and attach to it */
	template <class SceneComponentClass>
	SceneComponentClass* SpawnChildComponent(USkeletalMeshComponent* Component, const FString Name,
	                                         const FName SocketName = "",
	                                         const FTransform RelativeTransform = FTransform::Identity);

	/** When no merger namespace, attach base meshes from CustomizationElementaryModules and process their attachments and materials */
	void ProcessAttachmentModule(const FName SocketName, TArray<UCustomizationElementaryAsset*>& SocketNameAssets,
	                             USkeletalMeshComponent*
	                             SkeletalMeshParentComponent,
	                             TMap<FString, UCustomizationMaterialAsset*>& MaterialNamespacesToData);

	/** Merge meshes within one merger namespace and process their attachments and materials */
	void ProcessMeshMergeModule(const FString Namespace, TArray<UCustomizationElementaryAsset*>& NamespaceAssets,
	                            USkeletalMeshComponent*
	                            SkeletalMeshParentComponent,
	                            TMap<FString, UCustomizationMaterialAsset*>& MaterialNamespacesToData);

	/** For given map of socket names to skeletal meshes for attachment, attach to the Component */
	void ProcessSkeletalAttachesForComponent(USkeletalMeshComponent* Component,
	                                         const TArray<FCustomizationElementarySubmoduleSkeletal>& MeshesForAttach,
	                                         const FString NameEnding,
	                                         TMap<FString, UCustomizationMaterialAsset*>& MaterialNamespacesToData);

	/** For given array of socket names and skeletal meshes for attachment, attach to the Component */
	void ProcessStaticAttachesForComponent(USkeletalMeshComponent* Component,
	                                       const TArray<FCustomizationElementarySubmoduleStatic>& MeshesForAttach,
	                                       const FString NameEnding,
	                                       TMap<FString, UCustomizationMaterialAsset*>& MaterialNamespacesToData);

	/** Check if socket exists, if it does, return socket name, else return NAME_None and print warning */
	template <class ComponentClass>
	static FName GetExistingSocketNameOrNameNone(const ComponentClass* Component, FName SocketName);

	/** Load CustomizationLoaderAsset. Note that previous loaded meshes won't be destroyed,
	 * you should call UnloadPreviousCustomization for that */
	UFUNCTION(BlueprintCallable)
	void LoadFromAsset(
		TArray<UCustomizationElementaryAsset*> NewElementaryAssets,
		TArray<UCustomizationMaterialAsset*> NewMaterialConfigs);

	UFUNCTION(BlueprintCallable)
	void UnloadPreviousCustomization();

public:
	UCustomizationLoaderComponent();

	/** LoadFromAsset on BeginPlay */
	virtual void BeginPlay() override;
};
