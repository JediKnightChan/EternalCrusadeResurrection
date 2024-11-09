// Copyleft: All rights reversed


#include "Customization/CustomizationLoaderComponent.h"

#include "Customization/CustomizationElementaryAsset.h"
#include "Customization/CustomizationLoaderAsset.h"
#include "Customization/CustomizationMaterialAsset.h"
#include "Customization/CustomizationMaterialNameSpace.h"
#include "CustomizationUtilsLibrary.h"
#include "MeshMergeFunctionLibrary.h"
#include "Customization/CustomizationAttachmentAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"


UCustomizationLoaderComponent::UCustomizationLoaderComponent()
{
	bInheritParentAnimations = true;
	bUseParentSkeleton = true;
	bLoadOnBeginPlay = false;
	bDebugLoading = false;
}

void UCustomizationLoaderComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bLoadOnBeginPlay)
	{
		TMap<UCustomizationElementaryAsset*, FCustomizationMaterialAssetMap> NewMaterialConfigsOverrides = {};
		TMap<UCustomizationElementaryAsset*, FCustomizationAttachmentAssetArray> NewExternalAttachments = {};
		LoadFromAsset({}, {}, NewMaterialConfigsOverrides, NewExternalAttachments);
	}
}


template <class SceneComponentClass>
SceneComponentClass* UCustomizationLoaderComponent::SpawnChildComponent(USkeletalMeshComponent* Component,
                                                                        const FString Name, const FName SocketName,
                                                                        const FTransform RelativeTransform)
{
	SceneComponentClass* ChildComponent = NewObject<SceneComponentClass>(
		Component, SceneComponentClass::StaticClass(), NAME_None);

	// Don't tick every frame customization component
	ChildComponent->SetComponentTickInterval(1.0f);

	ChildComponent->RegisterComponent();
	ChildComponent->SetRelativeTransform(RelativeTransform);
	if (!SocketName.IsNone())
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
                                                  TArray<UCustomizationMaterialAsset*> NewMaterialConfigs,
                                                  const TMap<UCustomizationElementaryAsset*,
                                                             FCustomizationMaterialAssetMap>&
                                                  NewMaterialConfigsOverrides,
                                                  const TMap<UCustomizationElementaryAsset*,
                                                             FCustomizationAttachmentAssetArray>&
                                                  NewExternalAttachments)
{
	// Setting new elementary assets if passed or old from config

	TMap<FName, TArray<UCustomizationElementaryAsset*>> MergeNamespaceToModules;
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
	TMap<FName, UCustomizationMaterialAsset*> MaterialNamespacesToData = {};
	for (UCustomizationMaterialAsset* Config : NewMaterialConfigs)
	{
		if (!Config)
		{
			UE_LOG(LogAssetData, Warning, TEXT("Material config is null on %s"), *(GetNameSafe(GetOwner())));
			continue;
		}

		MaterialNamespacesToData.Add(Config->MaterialNamespace, Config);
	}

	if (bDebugLoading)
	{
		UE_LOG(LogTemp, Display, TEXT("DEBUG: CLC STARTING LOAD"))
	}

	// Fill array of modules for attachment and merge 
	for (UCustomizationElementaryAsset* ElementaryAsset : NewElementaryAssets)
	{
		if (!ElementaryAsset)
		{
			continue;
		}

		if (!ElementaryAsset->MeshMergeNamespace.IsNone() && NewMaterialConfigsOverrides.FindRef(ElementaryAsset).Map.
			IsEmpty())
		{
			if (bDebugLoading)
			{
				UE_LOG(LogTemp, Display, TEXT("DEBUG: Adding CEA %s to merger namespace %s"),
				       *(GetNameSafe(ElementaryAsset)),
				       *(ElementaryAsset->MeshMergeNamespace.ToString()))
			}
			MergeNamespaceToModules.FindOrAdd(ElementaryAsset->MeshMergeNamespace).Add(ElementaryAsset);
		}
		else
		{
			if (bDebugLoading)
			{
				UE_LOG(LogTemp, Display, TEXT("DEBUG: Adding CEA %s as separate asset"),
				       *(GetNameSafe(ElementaryAsset)))
			}
			AttachSocketNameToModules.FindOrAdd(ElementaryAsset->ParentAttachName).Add(ElementaryAsset);
		}
	}

	// Processing mesh merges
	for (TTuple<FName, TArray<UCustomizationElementaryAsset*>>& MergeNamespaceAndModule : MergeNamespaceToModules)
	{
		ProcessMeshMergeModule(MergeNamespaceAndModule.Key, MergeNamespaceAndModule.Value, SkeletalMeshParentComponent,
		                       MaterialNamespacesToData, NewExternalAttachments);
	}

	// Processing mesh attaches
	for (TTuple<FName, TArray<UCustomizationElementaryAsset*>> AttachSocketNameAndModule : AttachSocketNameToModules)
	{
		ProcessAttachmentModule(AttachSocketNameAndModule.Key, AttachSocketNameAndModule.Value,
		                        SkeletalMeshParentComponent, MaterialNamespacesToData, NewMaterialConfigsOverrides,
		                        NewExternalAttachments);
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
	if (SocketName.IsNone())
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
                                                                        TMap<FName, UCustomizationMaterialAsset*>&
                                                                        MaterialNamespacesToData)
{
	// Attaching skeletal meshes to sockets
	for (auto [SkeletalMesh, SocketName, SocketTransform, DefaultCustomizationNamespace, SlotNamesToNamespaces] :
	     MeshesForAttach)
	{
		SocketName = GetExistingSocketNameOrNameNone(Component, SocketName);
		USkeletalMeshComponent* SkeletalMeshComponent = SpawnChildComponent<USkeletalMeshComponent>(
			Component, "",
			SocketName, SocketTransform);
		SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);

		if (!CollisionProfileName.IsNone())
		{
			SkeletalMeshComponent->SetCollisionProfileName(CollisionProfileName);
		}

		TArray<FName> MaterialSlotNames = SkeletalMeshComponent->GetMaterialSlotNames();
		for (auto SlotName : MaterialSlotNames)
		{
			FName CustomizationNamespace = DefaultCustomizationNamespace;
			if (SlotNamesToNamespaces.Contains(SlotName))
			{
				CustomizationNamespace = SlotNamesToNamespaces[SlotName];
			}

			if (MaterialNamespacesToData.Contains(CustomizationNamespace))
			{
				const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[CustomizationNamespace];
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display,
					       TEXT("DEBUG: Skeletal attach (%s): setting slot %s to namespace %s, CMA %s"),
					       *(GetNameSafe(SkeletalMesh)), *(SlotName.ToString()), *(CustomizationNamespace.ToString()),
					       *(GetNameSafe(MaterialData)))
				}

				UCustomizationMaterialNameSpace::ApplyMaterialChanges(SkeletalMeshComponent,
				                                                      MaterialData->ScalarParameters,
				                                                      MaterialData->VectorParameters,
				                                                      MaterialData->TextureParameters, {SlotName});
			}
			else
			{
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display,
					       TEXT(
						       "DEBUG: Skeletal attach (%s): FAILURE setting slot %s to namespace %s, missing on dict with length %d"
					       ),
					       *(GetNameSafe(SkeletalMesh)), *(SlotName.ToString()), *(CustomizationNamespace.ToString()),
					       MaterialNamespacesToData.Num())
				}
			}
		}
	}
}

void UCustomizationLoaderComponent::ProcessStaticAttachesForComponent(USkeletalMeshComponent* Component,
                                                                      const TArray<
	                                                                      FCustomizationElementarySubmoduleStatic>&
                                                                      MeshesForAttach,
                                                                      const FString NameEnding,
                                                                      TMap<FName, UCustomizationMaterialAsset*>&
                                                                      MaterialNamespacesToData)
{
	// Attaching skeletal meshes to sockets
	for (auto [StaticMesh, SocketName, SocketTransform, DefaultCustomizationNamespace, SlotNamesToNamespaces] :
	     MeshesForAttach)
	{
		SocketName = GetExistingSocketNameOrNameNone(Component, SocketName);
		UStaticMeshComponent* StaticMeshComponent = SpawnChildComponent<UStaticMeshComponent>(
			Component, "",
			SocketName, SocketTransform);
		StaticMeshComponent->SetStaticMesh(StaticMesh);

		if (!CollisionProfileName.IsNone())
		{
			StaticMeshComponent->SetCollisionProfileName(CollisionProfileName);
		}

		TArray<FName> MaterialSlotNames = StaticMeshComponent->GetMaterialSlotNames();
		for (auto SlotName : MaterialSlotNames)
		{
			FName CustomizationNamespace = DefaultCustomizationNamespace;
			if (SlotNamesToNamespaces.Contains(SlotName))
			{
				CustomizationNamespace = SlotNamesToNamespaces[SlotName];
			}

			// Applying material changes
			if (MaterialNamespacesToData.Contains(CustomizationNamespace))
			{
				const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[CustomizationNamespace];
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display, TEXT("DEBUG: Static attach (%s): setting slot %s to namespace %s, CMA %s"),
					       *(GetNameSafe(StaticMesh)), *(SlotName.ToString()), *(CustomizationNamespace.ToString()),
					       *(GetNameSafe(MaterialData)))
				}
				UCustomizationMaterialNameSpace::ApplyMaterialChanges(StaticMeshComponent,
				                                                      MaterialData->ScalarParameters,
				                                                      MaterialData->VectorParameters,
				                                                      MaterialData->TextureParameters, {SlotName});
			}
			else
			{
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display,
					       TEXT(
						       "DEBUG: Static attach (%s): FAILURE setting slot %s to namespace %s, missing on dict with length %d"
					       ),
					       *(GetNameSafe(StaticMesh)), *(SlotName.ToString()), *(CustomizationNamespace.ToString()),
					       MaterialNamespacesToData.Num())
				}
			}
		}
	}
}

void UCustomizationLoaderComponent::ProcessParticleAttachesForComponent(USkeletalMeshComponent* Component,
                                                                        const TArray<
	                                                                        FCustomizationElementarySubmoduleParticle>&
                                                                        ParticlesForAttach, const FString NameEnding)
{
	// Attaching particles to sockets
	for (auto [ParticleSystem, SocketName, SocketTransform] :
	     ParticlesForAttach)
	{
		SocketName = GetExistingSocketNameOrNameNone(Component, SocketName);
		UParticleSystemComponent* ParticleSystemComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem,
			Component,
			SocketName,
			SocketTransform.GetLocation(),
			SocketTransform.Rotator(),
			SocketTransform.GetScale3D(),
			EAttachLocation::SnapToTarget,
			false);
		if (ParticleSystemComponent)
		{
			SpawnedComponents.Add(ParticleSystemComponent);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Empty particle system"));
		}
	}
}


void UCustomizationLoaderComponent::ProcessMeshMergeModule(const FName Namespace,
                                                           TArray<UCustomizationElementaryAsset*>&
                                                           NamespaceAssets,
                                                           USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                           TMap<FName, UCustomizationMaterialAsset*>&
                                                           MaterialNamespacesToData,
                                                           const TMap<UCustomizationElementaryAsset*,
                                                                      FCustomizationAttachmentAssetArray>&
                                                           NewExternalAttachments)
{
	TArray<USkeletalMesh*> MeshesForMerge;
	TArray<FCustomizationElementarySubmoduleStatic> StaticMeshesForAttach;
	TArray<FCustomizationElementarySubmoduleSkeletal> SkeletalMeshesForAttach;
	TArray<FCustomizationElementarySubmoduleParticle> ParticlesForAttach;
	TMap<FName, TArray<FName>> MaterialNamespacesToSlotNames;

	for (const UCustomizationElementaryAsset* ElementaryAsset : NamespaceAssets)
	{
		if (ElementaryAsset->BaseSkeletalMesh)
		{
			MeshesForMerge.AddUnique(ElementaryAsset->BaseSkeletalMesh);
		}

		if (!ElementaryAsset->MaterialCustomizationNamespace.IsNone())
		{
			TArray<FName> NotOverridenSlotNames;
			for (auto SlotName : ElementaryAsset->MaterialCustomizationSlotNames)
			{
				if (!ElementaryAsset->SlotNamesToMaterialNamespaceOverrides.Contains(SlotName))
				{
					// Not overriden, use default material namespace
					MaterialNamespacesToSlotNames.FindOrAdd(ElementaryAsset->MaterialCustomizationNamespace).Add(
						SlotName);
				}
				else
				{
					// Namespace for this slot name is overriden
					MaterialNamespacesToSlotNames.FindOrAdd(
						ElementaryAsset->SlotNamesToMaterialNamespaceOverrides[SlotName]).Add(SlotName);
				}
			}
		}

		StaticMeshesForAttach.Append(ElementaryAsset->StaticAttachments);
		SkeletalMeshesForAttach.Append(ElementaryAsset->SkeletalAttachments);
		ParticlesForAttach.Append(ElementaryAsset->ParticleAttachments);

		FCustomizationAttachmentAssetArray CAAArray = NewExternalAttachments.FindRef(ElementaryAsset);
		if (!CAAArray.Array.IsEmpty())
		{
			for (UCustomizationAttachmentAsset* AttachmentAsset : CAAArray.Array)
			{
				if (AttachmentAsset)
				{
					StaticMeshesForAttach.Append(AttachmentAsset->StaticAttachments);
					SkeletalMeshesForAttach.Append(AttachmentAsset->SkeletalAttachments);
					ParticlesForAttach.Append(AttachmentAsset->ParticleAttachments);
				}
			}
		}
	}

	USkeletalMesh* MergedSkeletalMesh = nullptr;
	if (MeshesForMerge.Num() > 1)
	{
		// Merging skeletal meshes
		FSkeletalMeshMergeParams MergeParams;
		if (bUseParentSkeleton && SkeletalMeshParentComponent->GetSkeletalMeshAsset())
		{
			MergeParams.Skeleton = SkeletalMeshParentComponent->GetSkeletalMeshAsset()->GetSkeleton();
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
		SkeletalMeshParentComponent, "");
	ChildComponent->SetSkeletalMesh(MergedSkeletalMesh);

	if (!CollisionProfileName.IsNone())
	{
		ChildComponent->SetCollisionProfileName(CollisionProfileName);
	}

	TArray<FName> Names = ChildComponent->GetMaterialSlotNames();

	// Inheriting animations if needed
	if (bInheritParentAnimations)
	{
		ChildComponent->SetLeaderPoseComponent(SkeletalMeshParentComponent);
	}

	// Applying materials
	for (TTuple<FName, TArray<FName>> MaterialNamespaceAndSlotNames : MaterialNamespacesToSlotNames)
	{
		if (MaterialNamespacesToData.Contains(MaterialNamespaceAndSlotNames.Key))
		{
			if (bDebugLoading)
			{
				for (FName SlotName : MaterialNamespaceAndSlotNames.Value)
				{
					UE_LOG(LogTemp, Display, TEXT("DEBUG: Mesh merge: Setting slot %s to namespace %s"),
					       *(SlotName.ToString()), *(MaterialNamespaceAndSlotNames.Key.ToString()))
				}
			}

			const UCustomizationMaterialAsset* MaterialData = MaterialNamespacesToData[MaterialNamespaceAndSlotNames.
				Key];
			UCustomizationMaterialNameSpace::ApplyMaterialChanges(ChildComponent, MaterialData->ScalarParameters,
			                                                      MaterialData->VectorParameters,
			                                                      MaterialData->TextureParameters,
			                                                      MaterialNamespaceAndSlotNames.Value);
		}
	}

	// Process attaching static meshes to child component
	ProcessStaticAttachesForComponent(ChildComponent, StaticMeshesForAttach, "", MaterialNamespacesToData);

	// Process attaching skeletal meshes to child component
	ProcessSkeletalAttachesForComponent(ChildComponent, SkeletalMeshesForAttach, "", MaterialNamespacesToData);

	// Process attaching particles
	ProcessParticleAttachesForComponent(ChildComponent, ParticlesForAttach, "");
}


void UCustomizationLoaderComponent::ProcessAttachmentModule(FName SocketName,
                                                            TArray<UCustomizationElementaryAsset*>& SocketNameAssets,
                                                            USkeletalMeshComponent* SkeletalMeshParentComponent,
                                                            TMap<FName, UCustomizationMaterialAsset*>&
                                                            MaterialNamespacesToData,
                                                            const TMap<UCustomizationElementaryAsset*,
                                                                       FCustomizationMaterialAssetMap>&
                                                            NewMaterialConfigsOverrides,
                                                            const TMap<UCustomizationElementaryAsset*,
                                                                       FCustomizationAttachmentAssetArray>&
                                                            NewExternalAttachments)
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
			ChildComponent->SetLeaderPoseComponent(SkeletalMeshParentComponent);
		}

		// Applying materials
		TArray<FName> MaterialSlotNames = ChildComponent->GetMaterialSlotNames();
		for (auto SlotName : MaterialSlotNames)
		{
			FName CustomizationNamespace = Asset->MaterialCustomizationNamespace;
			if (Asset->SlotNamesToMaterialNamespaceOverrides.Contains(SlotName))
			{
				CustomizationNamespace = Asset->SlotNamesToMaterialNamespaceOverrides[SlotName];
			}

			if (bDebugLoading)
			{
				UE_LOG(LogTemp, Display, TEXT("DEBUG: CEA attachment (%s): setting slot %s to namespace %s"),
				       *(GetNameSafe(Asset)), *(SlotName.ToString()), *(CustomizationNamespace.ToString()))
			}

			const UCustomizationMaterialAsset* MaterialData = nullptr;
			if (NewMaterialConfigsOverrides.FindRef(Asset).Map.Contains(CustomizationNamespace))
			{
				MaterialData = NewMaterialConfigsOverrides.FindRef(Asset).Map[CustomizationNamespace];
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display, TEXT("DEBUG: CEA attachment (%s): using overriden CMA %s"),
					       *(GetNameSafe(Asset)), *(GetNameSafe(MaterialData)))
				}
			}
			else if (MaterialNamespacesToData.Contains(CustomizationNamespace))
			{
				MaterialData = MaterialNamespacesToData[CustomizationNamespace];
				if (bDebugLoading)
				{
					UE_LOG(LogTemp, Display, TEXT("DEBUG: CEA attachment (%s): using default CMA %s"),
					       *(GetNameSafe(Asset)), *(GetNameSafe(MaterialData)))
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Couldn't find material namespace %s for %s"),
				       *(CustomizationNamespace.ToString()),
				       *(UKismetSystemLibrary::GetDisplayName(this)))
			}

			if (MaterialData)
			{
				UCustomizationMaterialNameSpace::ApplyMaterialChanges(
					ChildComponent, MaterialData->ScalarParameters,
					MaterialData->VectorParameters,
					MaterialData->TextureParameters, {SlotName});
			}
		}

		TMap<FName, UCustomizationMaterialAsset*> OverridenMaterialNamespacesToData = MaterialNamespacesToData;
		// Overriding material data with override map
		for (TTuple<FName, UCustomizationMaterialAsset*> MaterialOverrideTuple : NewMaterialConfigsOverrides.
		     FindRef(Asset).Map)
		{
			OverridenMaterialNamespacesToData.Add(MaterialOverrideTuple.Key, MaterialOverrideTuple.Value);
		}

		// Process attaching static meshes to child component
		ProcessStaticAttachesForComponent(ChildComponent, Asset->StaticAttachments, Namespace,
		                                  OverridenMaterialNamespacesToData);

		// Process attaching skeletal meshes to child component
		ProcessSkeletalAttachesForComponent(ChildComponent, Asset->SkeletalAttachments, Namespace,
		                                    OverridenMaterialNamespacesToData);

		// Process attaching particle systems to child component
		ProcessParticleAttachesForComponent(ChildComponent, Asset->ParticleAttachments, Namespace);

		// Process external attachments (CAAs)
		FCustomizationAttachmentAssetArray CustomizationAttachmentAssetArray = NewExternalAttachments.FindRef(Asset);
		if (!CustomizationAttachmentAssetArray.Array.IsEmpty())
		{
			for (UCustomizationAttachmentAsset* CAA : CustomizationAttachmentAssetArray.Array)
			{
				if (CAA)
				{
					ProcessStaticAttachesForComponent(ChildComponent, CAA->StaticAttachments, Namespace,
					                                  OverridenMaterialNamespacesToData);
					ProcessSkeletalAttachesForComponent(ChildComponent, CAA->SkeletalAttachments, Namespace,
					                                    OverridenMaterialNamespacesToData);
					ProcessParticleAttachesForComponent(ChildComponent, CAA->ParticleAttachments, Namespace);
				}
			}
		}
	}
}
