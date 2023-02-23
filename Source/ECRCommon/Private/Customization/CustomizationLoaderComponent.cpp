// Copyleft: All rights reversed


#include "Customization/CustomizationLoaderComponent.h"

#include "Customization/CustomizationElementaryAsset.h"
#include "Customization/CustomizationLoaderAsset.h"
#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationMaterialNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "MeshMergeFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


UCustomizationLoaderComponent::UCustomizationLoaderComponent()
{
	bInheritParentAnimations = true;
	bUseParentSkeleton = true;
	bLoadOnBeginPlay = false;
}

void UCustomizationLoaderComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bLoadOnBeginPlay)
	{
		LoadFromAsset({}, {});
	}
}


template <class SceneComponentClass>
SceneComponentClass* UCustomizationLoaderComponent::SpawnChildComponent(USkeletalMeshComponent* Component,
                                                                        const FString Name, const FName SocketName,
                                                                        const FTransform RelativeTransform)
{
	SceneComponentClass* ChildComponent = NewObject<SceneComponentClass>(
		Component, SceneComponentClass::StaticClass(), FName{Name});

	ChildComponent->RegisterComponent();
	ChildComponent->SetRelativeTransform(RelativeTransform);
	if (SocketName != "" && SocketName != "None")
	{
		ChildComponent->AttachToComponent(Component, FAttachmentTransformRules::KeepRelativeTransform,
		                                  SocketName);
	}
	else
	{
		ChildComponent->AttachToComponent(Component, FAttachmentTransformRules::KeepRelativeTransform);
	}
	ChildComponent->CreationMethod = EComponentCreationMethod::Native;

	SpawnedComponents.Add(ChildComponent);
	return ChildComponent;
}


void UCustomizationLoaderComponent::LoadFromAsset(TArray<UCustomizationElementaryAsset*> NewElementaryAssets,
                                                  TArray<UCustomizationMaterialAsset*> NewMaterialConfigs)
{
	// Setting new elementary assets if passed or old from config
	TArray<UCustomizationElementaryAsset*> ElementaryAssets;
	if (!NewElementaryAssets.IsEmpty())
	{
		ElementaryAssets = NewElementaryAssets;
	}
	else
	{
		// Use elementary assets from asset config
		if (AssetConfig == nullptr)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("AssetConfig of CustomizationLoaderComponent %s is not specified, but tried to load it. "
				       "Stopping loading it"),
			       *(UKismetSystemLibrary::GetDisplayName(this)))
			return;
		}
		AssetConfig->GetAssetsAsMap().GenerateValueArray(ElementaryAssets);
	}

	// Setting materials if overriden
	if (!NewMaterialConfigs.IsEmpty())
	{
		MaterialConfigs = NewMaterialConfigs;
	}

	TMap<FString, TArray<UCustomizationElementaryAsset*>> MergeNamespaceToModules;
	TMap<FName, TArray<UCustomizationElementaryAsset*>> AttachSocketNameToModules;

	// Retrieving first parent skeletal mesh: for attaching created components to it
	USkeletalMeshComponent* SkeletalMeshParentComponent = UCustomizationUtilsLibrary::GetFirstParentComponentOfType<
		USkeletalMeshComponent>(this);
	if (SkeletalMeshParentComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning,
		       TEXT("Couldn't find SkeletalMeshComponent among parents of %s, exiting"),
		       *(UKismetSystemLibrary::GetDisplayName(this)))
		return;
	}

	// Material namespace data
	TMap<FString, UCustomizationMaterialAsset*> MaterialNamespacesToData;
	for (UCustomizationMaterialAsset* Config : MaterialConfigs)
	{
		if (!Config)
		{
			UE_LOG(LogAssetData, Warning, TEXT("Material config is null on %s"), *(GetNameSafe(GetOwner())));
			continue;
		}

		MaterialNamespacesToData.Add(Config->MaterialNamespace, Config);
	}

	// Fill array of modules for attachment and merge 
	for (UCustomizationElementaryAsset* ElementaryAsset : ElementaryAssets)
	{
		if (!ElementaryAsset)
		{
			if (NewElementaryAssets.IsEmpty())
			{
				UE_LOG(LogAssetData, Warning, TEXT("Elementary asset is null in %s"), *(GetNameSafe(AssetConfig)));
			}
			continue;
		}

		if (ElementaryAsset->MeshMergeNamespace != "")
		{
			MergeNamespaceToModules.FindOrAdd(ElementaryAsset->MeshMergeNamespace).Add(ElementaryAsset);
		}
		else
		{
			AttachSocketNameToModules.FindOrAdd(ElementaryAsset->ParentAttachName).Add(ElementaryAsset);
		}
	}

	// Processing mesh merges
	for (TTuple<FString, TArray<UCustomizationElementaryAsset*>>& MergeNamespaceAndModule : MergeNamespaceToModules)
	{
		ProcessMeshMergeModule(MergeNamespaceAndModule.Key, MergeNamespaceAndModule.Value, SkeletalMeshParentComponent,
		                       MaterialNamespacesToData);
	}

	// Processing mesh attaches
	for (TTuple<FName, TArray<UCustomizationElementaryAsset*>> AttachSocketNameAndModule : AttachSocketNameToModules)
	{
		ProcessAttachmentModule(AttachSocketNameAndModule.Key, AttachSocketNameAndModule.Value,
		                        SkeletalMeshParentComponent, MaterialNamespacesToData);
	}
}

void UCustomizationLoaderComponent::UnloadPreviousCustomization()
{
	for (USceneComponent* SpawnedComponent : SpawnedComponents)
	{
		if (SpawnedComponent)
		{
			SpawnedComponent->DestroyComponent();
		}
	}
	SpawnedComponents.Empty();
}


template <class ComponentClass>
FName UCustomizationLoaderComponent::GetExistingSocketNameOrNameNone(const ComponentClass* Component,
                                                                     FName SocketName)
{
	if (SocketName == "" || SocketName == NAME_None)
	{
		SocketName = NAME_None;
	}
	else if (!Component->DoesSocketExist(SocketName))
	{
		SocketName = NAME_None;
		UE_LOG(LogTemp, Warning, TEXT("Socket name %s does not exist on skeletal mesh attachment"),
		       *SocketName.FName::ToString())
	}
	return SocketName;
}

void UCustomizationLoaderComponent::ProcessSkeletalAttachesForComponent(USkeletalMeshComponent* Component,
                                                                        const TArray<
	                                                                        FCustomizationElementarySubmoduleSkeletal>&
                                                                        MeshesForAttach,
                                                                        const FString NameEnding,
                                                                        TMap<FString, UCustomizationMaterialAsset*>&
                                                                        MaterialNamespacesToData)
{
	// Attaching skeletal meshes to sockets
	for (auto [SkeletalMesh, SocketName, SocketTransform, CustomizationNamespace] : MeshesForAttach)
	{
		SocketName = GetExistingSocketNameOrNameNone(Component, SocketName);
		USkeletalMeshComponent* SkeletalMeshComponent = SpawnChildComponent<USkeletalMeshComponent>(
			Component, NameEnding + SocketName.ToString(),
			SocketName, SocketTransform);
		SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);

		if (!CollisionProfileName.IsNone())
		{
			SkeletalMeshComponent->SetCollisionProfileName(CollisionProfileName);
		}

		if (MaterialNamespacesToData.Contains(CustomizationNamespace))
		{
			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[CustomizationNamespace];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(SkeletalMeshComponent,
			                                                      MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters, {});
		}
	}
}

void UCustomizationLoaderComponent::ProcessStaticAttachesForComponent(USkeletalMeshComponent* Component,
                                                                      const TArray<
	                                                                      FCustomizationElementarySubmoduleStatic>&
                                                                      MeshesForAttach,
                                                                      const FString NameEnding,
                                                                      TMap<FString, UCustomizationMaterialAsset*>&
                                                                      MaterialNamespacesToData)
{
	// Attaching skeletal meshes to sockets
	for (auto [StaticMesh, SocketName, SocketTransform, CustomizationNamespace] : MeshesForAttach)
	{
		SocketName = GetExistingSocketNameOrNameNone(Component, SocketName);
		UStaticMeshComponent* StaticMeshComponent = SpawnChildComponent<UStaticMeshComponent>(
			Component, NameEnding + SocketName.ToString(),
			SocketName, SocketTransform);
		StaticMeshComponent->SetStaticMesh(StaticMesh);

		if (!CollisionProfileName.IsNone())
		{
			StaticMeshComponent->SetCollisionProfileName(CollisionProfileName);
		}


		// Applying material changes
		if (MaterialNamespacesToData.Contains(CustomizationNamespace))
		{
			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[CustomizationNamespace];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(StaticMeshComponent,
			                                                      MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters, {});
		}
	}
}


void UCustomizationLoaderComponent::ProcessMeshMergeModule(const FString Namespace,
                                                           TArray<UCustomizationElementaryAsset*>&
                                                           NamespaceAssets,
                                                           USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                           TMap<FString, UCustomizationMaterialAsset*>&
                                                           MaterialNamespacesToData)
{
	TArray<USkeletalMesh*> MeshesForMerge;
	TArray<FCustomizationElementarySubmoduleStatic> StaticMeshesForAttach;
	TArray<FCustomizationElementarySubmoduleSkeletal> SkeletalMeshesForAttach;
	TMap<FString, TArray<FName>> MaterialNamespacesToSlotNames;

	for (const UCustomizationElementaryAsset* ElementaryAsset : NamespaceAssets)
	{
		if (ElementaryAsset->BaseSkeletalMesh)
		{
			MeshesForMerge.AddUnique(ElementaryAsset->BaseSkeletalMesh);
		}

		if (ElementaryAsset->MaterialCustomizationNamespace != "")
		{
			MaterialNamespacesToSlotNames.FindOrAdd(ElementaryAsset->MaterialCustomizationNamespace).Append(
				ElementaryAsset->MaterialCustomizationSlotNames);
		}

		StaticMeshesForAttach.Append(ElementaryAsset->StaticAttachments);
		SkeletalMeshesForAttach.Append(ElementaryAsset->SkeletalAttachments);
	}

	USkeletalMesh* MergedSkeletalMesh = nullptr;
	if (MeshesForMerge.Num() > 1)
	{
		// Merging skeletal meshes
		FSkeletalMeshMergeParams MergeParams;
		if (bUseParentSkeleton && SkeletalMeshParentComponent->SkeletalMesh)
		{
			MergeParams.Skeleton = SkeletalMeshParentComponent->SkeletalMesh->GetSkeleton();
			MergeParams.bSkeletonBefore = false;
		}
		MergeParams.MeshesToMerge = MeshesForMerge;
		MergedSkeletalMesh = UMeshMergeFunctionLibrary::MergeMeshes(MergeParams);
	}
	else if (MeshesForMerge.Num() == 1)
	{
		MergedSkeletalMesh = MeshesForMerge[0];
	}

	if (MergedSkeletalMesh == nullptr)
	{
		return;
	}

	// Creating child component for merged mesh and setting merged mesh as its mesh
	USkeletalMeshComponent* ChildComponent = SpawnChildComponent<USkeletalMeshComponent>(
		SkeletalMeshParentComponent, Namespace);
	ChildComponent->SetSkeletalMesh(MergedSkeletalMesh);

	if (!CollisionProfileName.IsNone())
	{
		ChildComponent->SetCollisionProfileName(CollisionProfileName);
	}

	TArray<FName> Names = ChildComponent->GetMaterialSlotNames();

	// Inheriting animations if needed
	if (bInheritParentAnimations)
	{
		ChildComponent->SetMasterPoseComponent(SkeletalMeshParentComponent);
	}

	// Applying materials
	for (TTuple<FString, TArray<FName>> MaterialNamespaceAndSlotNames : MaterialNamespacesToSlotNames)
	{
		if (MaterialNamespacesToData.Contains(MaterialNamespaceAndSlotNames.Key))
		{
			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[MaterialNamespaceAndSlotNames.
				Key];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters,
			                                                      MaterialNamespaceAndSlotNames.Value);
		}
	}

	// Process attaching static meshes to child component
	ProcessStaticAttachesForComponent(ChildComponent, StaticMeshesForAttach, Namespace, MaterialNamespacesToData);

	// Process attaching skeletal meshes to child component
	ProcessSkeletalAttachesForComponent(ChildComponent, SkeletalMeshesForAttach, Namespace, MaterialNamespacesToData);
}


void UCustomizationLoaderComponent::ProcessAttachmentModule(FName SocketName,
                                                            TArray<UCustomizationElementaryAsset*>& SocketNameAssets,
                                                            USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                            TMap<FString, UCustomizationMaterialAsset*>&
                                                            MaterialNamespacesToData)
{
	for (const UCustomizationElementaryAsset* Asset : SocketNameAssets)
	{
		const FString Namespace{
			UKismetSystemLibrary::GetDisplayName(Asset->BaseSkeletalMesh) + SocketName.ToString()
		};

		// Spawning child component attached to SocketName and setting its skeletal mesh
		SocketName = GetExistingSocketNameOrNameNone(SkeletalMeshParentComponent, SocketName);
		USkeletalMeshComponent* ChildComponent = SpawnChildComponent<USkeletalMeshComponent>(
			SkeletalMeshParentComponent, Namespace, SocketName);
		ChildComponent->SetSkeletalMesh(Asset->BaseSkeletalMesh);

		if (!CollisionProfileName.IsNone())
		{
			ChildComponent->SetCollisionProfileName(CollisionProfileName);
		}

		// Inheriting animations if needed
		if (bInheritParentAnimations)
		{
			ChildComponent->SetMasterPoseComponent(SkeletalMeshParentComponent);
		}

		// Applying materials
		if (MaterialNamespacesToData.Contains(Asset->MaterialCustomizationNamespace))
		{
			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[Asset->
				MaterialCustomizationNamespace];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters, {});
		}

		// Process attaching static meshes to child component
		ProcessStaticAttachesForComponent(ChildComponent, Asset->StaticAttachments, Namespace,
		                                  MaterialNamespacesToData);

		// Process attaching skeletal meshes to child component
		ProcessSkeletalAttachesForComponent(ChildComponent, Asset->SkeletalAttachments, Namespace,
		                                    MaterialNamespacesToData);
	}
}
