// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Customization/CustomizationMaterialAsset.h"
#include "CustomizationElementaryAsset.generated.h"


/** Container to store static mesh component attached to UCustomizationElementaryModel */
USTRUCT(BlueprintType)
struct ECRCOMMON_API FCustomizationElementarySubmoduleStatic
{
	GENERATED_BODY()

	/** Static mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UStaticMesh* StaticMesh = nullptr;

	/** Socket name on UCustomizationElementaryModel to attach mesh to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	/** Relative transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform AttachTransform;

	/** Default customization namespace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CustomizationNamespace;

	/** Overrides of material namespace for concise slot names */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<FName, FString> SlotNamesToMaterialNamespaceOverrides;
};


/** Container to store skeletal mesh component attached to UCustomizationElementaryModel */
USTRUCT(BlueprintType)
struct ECRCOMMON_API FCustomizationElementarySubmoduleSkeletal
{
	GENERATED_BODY()

	/** Static mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USkeletalMesh* SkeletalMesh = nullptr;

	/** Socket name on UCustomizationElementaryModel to attach mesh to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName;

	/** Relative transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform AttachTransform;

	/** Default customization namespace */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString CustomizationNamespace;

	/** Overrides of material namespace for concise slot names */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<FName, FString> SlotNamesToMaterialNamespaceOverrides;
};


/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationElementaryAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FString ModuleName;

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

	/** Slot names that will be used for applying materials if this mesh is going to be merged with others */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FName> MaterialCustomizationSlotNames;

	/** Overrides of material namespace for concise slot names */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TMap<FName, FString> SlotNamesToMaterialNamespaceOverrides;
	
	/** Array of static attachments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<FCustomizationElementarySubmoduleStatic> StaticAttachments;

	/** Array of skeletal attachments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<FCustomizationElementarySubmoduleSkeletal> SkeletalAttachments;
	
	/** Overrides for material data. WARNING: this will prevent mesh merging (MeshMergeNamespace will be set to None) */
	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	TMap<FString, UCustomizationMaterialAsset*> MaterialOverrides;
};
