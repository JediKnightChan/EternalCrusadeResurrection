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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Layer))
	TArray<FECRAnimLayerSelectionEntry> LayerRules;

	// The layer to use if none of the LayerRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> DefaultLayer;

	// Choose the best layer given the rules
	TSubclassOf<UAnimInstance> SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const;
};

//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRAnimBodyStyleSelectionEntry
{
	GENERATED_BODY()

	// Skeletal mesh to apply if the tag matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> Mesh = nullptr;

	// Cosmetic tags required (all of these must be present to be considered a match)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

USTRUCT(BlueprintType)
struct FECRAnimBodyStyleSelectionSet
{
	GENERATED_BODY()

	// List of layer rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Mesh))
	TArray<FECRAnimBodyStyleSelectionEntry> MeshRules;

	// The layer to use if none of the MeshRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> DefaultMesh = nullptr;

	// If set, ensures this physics asset is always used
	UPROPERTY(EditAnywhere)
	TObjectPtr<UPhysicsAsset> ForcedPhysicsAsset = nullptr;

	// Choose the best body style skeletal mesh given the rules
	USkeletalMesh* SelectBestBodyStyle(const FGameplayTagContainer& CosmeticTags) const;
};


//////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FECRAnimMontageSelectionEntry
{
	GENERATED_BODY()

	// Layer to apply if the tag matches
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

	// List of layer rules to apply, first one that matches will be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Mesh))
	TArray<FECRActorSelectionEntry> ActorRules;

	// The layer to use if none of the MeshRules matches
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> DefaultActorClass = nullptr;

	// Choose the best actor class given the rules
	TSubclassOf<AActor> SelectBestActor(const FGameplayTagContainer& CosmeticTags) const;
};