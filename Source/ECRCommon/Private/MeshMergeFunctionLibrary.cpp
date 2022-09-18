// Fill out your copyright notice in the Description page of Project Settings.

#include "MeshMergeFunctionLibrary.h"
#include "SkeletalMeshMerge.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"


USkeletalMesh* UMeshMergeFunctionLibrary::MergeMeshes(const FSkeletalMeshMergeParams& Params)
{
	TArray<USkeletalMesh*> MeshesToMergeCopy = Params.MeshesToMerge;
	MeshesToMergeCopy.RemoveAll([](const USkeletalMesh* InMesh)
	{
		return InMesh == nullptr;
	});
	if (MeshesToMergeCopy.Num() <= 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Must provide multiple valid Skeletal Meshes in order to perform a merge."));
		return nullptr;
	}
	const EMeshBufferAccess BufferAccess = Params.bNeedsCpuAccess
		                                       ? EMeshBufferAccess::ForceCPUAndGPU
		                                       : EMeshBufferAccess::Default;
	const TArray<FSkelMeshMergeSectionMapping> SectionMappings;
	
	USkeletalMesh* BaseMesh = NewObject<USkeletalMesh>();
	if (Params.Skeleton && Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);
	}

	FSkeletalMeshMerge Merger(BaseMesh, MeshesToMergeCopy, SectionMappings, Params.StripTopLODS, BufferAccess);
	if (!Merger.DoMerge())
	{
		UE_LOG(LogTemp, Warning, TEXT("Merge failed!"));
		return nullptr;
	}
	if (Params.Skeleton && !Params.bSkeletonBefore)
	{
		BaseMesh->SetSkeleton(Params.Skeleton);
	}
	return BaseMesh;
}
