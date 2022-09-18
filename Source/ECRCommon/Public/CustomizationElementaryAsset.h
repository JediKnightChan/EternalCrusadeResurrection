// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CustomizationElementaryAsset.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationElementaryAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** The base of this module - Skeletal mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USkeletalMesh* BaseSkeletalMesh;

	/** Socket name to attach the base skeletal mesh to if attachment is required */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FName ParentAttachName;

	/** Namespace shared between modules that require to be merged together */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FString MeshMergeNamespace;

	/** Namespace shared between modules that require to be merged together */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FString MaterialCustomizationNamespace;

	/* Map of static meshes to their socket names to attach to the base skeletal mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<class UStaticMesh*, FName> StaticMeshesToSocketNames;

	/* Map of skeletal meshes to their socket names to attach to the base skeletal mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<class USkeletalMesh*, FName> SkeletalMeshesToSocketNames;
};
