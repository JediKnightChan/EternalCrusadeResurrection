// Copyleft: All rights reversed

#include "ModularMeshLoaderComponent.h"

UModularMeshLoaderComponent::UModularMeshLoaderComponent()
{
	bInheritAnimations = true;
}

void UModularMeshLoaderComponent::BeginPlay()
{
	Super::BeginPlay();

	// // Setting animations like on it
	// if (FirstParentSkeletalMeshComponent)
	// {
	// 	SetMasterPoseComponent(FirstParentSkeletalMeshComponent, true);
	// }
	// else
	// {
	// 	UE_LOG(LogTemp, Warning,
	// 	       TEXT(
	// 		       "ModularMeshComponent %s wants to inherit animations, but none of its parents is a Skeletal Mesh"
	// 	       ),
	// 	       *(GetName()))
	// }
	//
	// FSkeletalMeshMergeParams MergeParams;
	//
	// TMap<FName, TMap<FName, FLinearColor>> MaterialsVectorData = {};
	// TMap<FName, TMap<FName, float>> MaterialsScalarData = {};
	// TMap<FName, TMap<FName, UTexture*>> MaterialsTextureData = {};
	//
	// // Save requested material parameter values for further reassigning
	// TArray<FName> MaterialNames = ChildSkeletalMeshComponent->GetMaterialSlotNames();
	// for (FName MaterialName : MaterialNames)
	// {
	// 	int32 MaterialIndex = ChildSkeletalMeshComponent->GetMaterialIndex(MaterialName);
	// 	UMaterialInterface* MaterialInterface = ChildSkeletalMeshComponent->GetMaterial(MaterialIndex);
	//
	// 	MaterialsScalarData.Add(MaterialName, {});
	// 	MaterialsVectorData.Add(MaterialName, {});
	// 	MaterialsTextureData.Add(MaterialName, {});
	//
	// 	// Save scalar parameters
	// 	for (FName ScalarChannelName : ScalarDynamicMaterialChannelsToCopy)
	// 	{
	// 		FMemoryImageMaterialParameterInfo Info = FMemoryImageMaterialParameterInfo(ScalarChannelName);
	// 		float Value;
	// 		MaterialInterface->GetScalarParameterValue(Info, Value);
	// 		MaterialsScalarData[MaterialName].Add(ScalarChannelName, Value);
	// 	}
	//
	// 	// Save vector parameters
	// 	for (FName VectorChannelName : VectorDynamicMaterialChannelsToCopy)
	// 	{
	// 		FMemoryImageMaterialParameterInfo Info = FMemoryImageMaterialParameterInfo(VectorChannelName);
	// 		FLinearColor Value;
	// 		MaterialInterface->GetVectorParameterValue(Info, Value);
	// 		MaterialsVectorData[MaterialName].Add(VectorChannelName, Value);
	// 	}
	//
	// 	// Save texture parameters
	// 	for (FName TextureChannelName : TextureDynamicMaterialChannelsToCopy)
	// 	{
	// 		FMemoryImageMaterialParameterInfo Info = FMemoryImageMaterialParameterInfo(TextureChannelName);
	// 		UTexture* Value;
	// 		MaterialInterface->GetTextureParameterValue(Info, Value);
	// 		MaterialsTextureData[MaterialName].Add(TextureChannelName, Value);
	// 	}
	// }
	//
	// // Merging skeletal meshes
	// MergeParams.Skeleton = SkeletalMesh->GetSkeleton();
	// MergeParams.bSkeletonBefore = false;
	// MergeParams.MeshesToMerge = MeshesForMerge;
	// USkeletalMesh* MergedSkeletalMesh = UMeshMergeFunctionLibrary::MergeMeshes(MergeParams);
	// SetSkeletalMesh(MergedSkeletalMesh);
	//
	// // Setting dynamic materials
	// TArray<FName> MaterialNames = GetMaterialSlotNames();
	// for (FName MaterialName : MaterialNames)
	// {
	// 	int32 MaterialIndex = GetMaterialIndex(MaterialName);
	// 	UMaterialInterface* MaterialInterface = GetMaterial(MaterialIndex);
	//
	// 	UMaterialInstanceDynamic* MaterialInstance = CreateDynamicMaterialInstance(
	// 		MaterialIndex, MaterialInterface);
	//
	// 	if (MaterialInstance == nullptr)
	// 	{
	// 		continue;
	// 	}
	//
	// 	// Setting scalar material data
	// 	if (MaterialsScalarData.Contains(MaterialName))
	// 	{
	// 		for (TTuple<FName, float> ChannelToValue : MaterialsScalarData[MaterialName])
	// 		{
	// 			MaterialInstance->SetScalarParameterValue(ChannelToValue.Key, ChannelToValue.Value);
	// 		}
	// 	}
	//
	// 	// Setting vector material data
	// 	if (MaterialsVectorData.Contains(MaterialName))
	// 	{
	// 		for (TTuple<FName, FLinearColor> ChannelToValue : MaterialsVectorData[MaterialName])
	// 		{
	// 			UE_LOG(LogTemp, Warning, TEXT("Vector value material %s, channel %s, value %s"),
	// 			       *(MaterialName.ToString()), *(ChannelToValue.Key.ToString()),
	// 			       *(ChannelToValue.Value.ToString()));
	// 			MaterialInstance->SetVectorParameterValue(ChannelToValue.Key, ChannelToValue.Value);
	// 		}
	// 	}
	//
	// 	// Setting texture material data
	// 	if (MaterialsTextureData.Contains(MaterialName))
	// 	{
	// 		for (TTuple<FName, UTexture*> ChannelToValue : MaterialsTextureData[MaterialName])
	// 		{
	// 			MaterialInstance->SetTextureParameterValue(ChannelToValue.Key, ChannelToValue.Value);
	// 		}
	// 	}
	//
	// 	UE_LOG(LogTemp, Warning, TEXT("New name %s"), *(MaterialName.ToString()));
	// }
	//
	// // Attaching static meshes to sockets
	// for (TTuple<TObjectPtr<UStaticMeshComponent>, FName> StaticMeshToSocketNamePair : StaticMeshComponentsForAttach)
	// {
	// 	if (DoesSocketExist(StaticMeshToSocketNamePair.Value))
	// 	{
	// 		StaticMeshToSocketNamePair.Key->AttachToComponent(this,
	// 		                                                  FAttachmentTransformRules::SnapToTargetNotIncludingScale,
	// 		                                                  StaticMeshToSocketNamePair.Value);
	// 	}
	// 	else
	// 	{
	// 		UE_LOG(LogTemp, Error, TEXT("Socket %s doesn't exist on generated mesh!"),
	// 		       *(StaticMeshToSocketNamePair.Value.ToString()))
	// 	}
	// }
}
