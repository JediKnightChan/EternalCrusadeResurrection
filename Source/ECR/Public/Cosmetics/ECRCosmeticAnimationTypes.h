// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"

#include "ECRCosmeticAnimationTypes.generated.h"

class USkeletalMesh;
class UPhysicsAsset;


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRSoftTexture2DSelectionEntry
{
	GENERATED_BODY()

	// Texture to apply if the tag matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Texture;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FECRSoftTexture2DSelectionSet
{
	GENERATED_BODY()

	// List of texture rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=RequiredTags))
	TArray<FECRSoftTexture2DSelectionEntry> TextureRules;

	// The texture to use if none of the LayerRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> DefaultTexture;

	// Choose the best texture given the rules
	TSoftObjectPtr<UTexture2D> SelectBestTexture(const FGameplayTagContainer& CosmeticTags) const;
};


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRAnimLayerSelectionEntry
{
	GENERATED_BODY()

	// Layer to apply if the tag matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> Layer;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FECRAnimLayerSelectionSet
{
	GENERATED_BODY()

	// List of layer rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=RequiredTags))
	TArray<FECRAnimLayerSelectionEntry> LayerRules;

	// The layer to use if none of the LayerRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> DefaultLayer;

	// Choose the best layer given the rules
	TSubclassOf<UAnimInstance> SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const;
};


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRAnimMontageSelectionEntry
{
	GENERATED_BODY()

	// Montage to apply if the tag matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UAnimMontage> Montage;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FECRAnimMontageSelectionSet
{
	GENERATED_BODY()

	// List of montage rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Montage))
	TArray<FECRAnimMontageSelectionEntry> MontageRules;

	// The montage to use if none of the LayerRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UAnimMontage> DefaultMontage;

	// Choose the best montage given the rules
	TSoftObjectPtr<UAnimMontage> SelectBestMontage(const FGameplayTagContainer& CosmeticTags) const;

	// Returns all montages associated with this set
	TArray<TSoftObjectPtr<UAnimMontage>> GetAllMontages();
};


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRActorSelectionEntry
{
	GENERATED_BODY()

	// Actor class to select if tags match
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass = nullptr;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FECRActorSelectionSet
{
	GENERATED_BODY()

	// List of actor rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=ActorClass))
	TArray<FECRActorSelectionEntry> ActorRules;

	// The actor to use if none of the ActorRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> DefaultActorClass = nullptr;

	// Choose the best actor class given the rules
	TSubclassOf<AActor> SelectBestActor(const FGameplayTagContainer& CosmeticTags) const;
};

//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRMeshSelectionEntry
{
	GENERATED_BODY()

	// Mesh class to select if tags match
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* MeshClass = nullptr;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};


USTRUCT(BlueprintType)
struct FECRMeshSelectionSet
{
	GENERATED_BODY()

	// List of mesh rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=RequiredTags))
	TArray<FECRMeshSelectionEntry> MeshRules;

	// The mesh to use if none of the MeshRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* DefaultMesh = nullptr;

	// Choose the best actor class given the rules
	USkeletalMesh* SelectBestMesh(const FGameplayTagContainer& CosmeticTags) const;
};


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRAnimInstanceSelectionEntry
{
	GENERATED_BODY()

	// Animation blueprint select if tags match
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimInstance = nullptr;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};


USTRUCT(BlueprintType)
struct FECRAnimInstanceSelectionSet
{
	GENERATED_BODY()

	// List of animation blueprint rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=AnimInstance))
	TArray<FECRAnimInstanceSelectionEntry> AnimInstanceRules;

	// The animation blueprint to use if none of the MeshRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> DefaultAnimInstance = nullptr;

	// Choose the best animation blueprint given the rules
	TSubclassOf<UAnimInstance> SelectBestAnimInstance(const FGameplayTagContainer& CosmeticTags) const;
};