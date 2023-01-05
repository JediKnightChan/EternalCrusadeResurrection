// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ECRPawnData.generated.h"

class APawn;
class UECRAbilitySet;
class UECRInputConfig;
class UECRAbilityTagRelationshipMapping;
class UECRCameraMode;


/**
 * UECRPawnData
 *
 *	Non-mutable data asset that contains properties used to define a pawn.
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "ECR Pawn Data", ShortTooltip = "Data asset used to define a Pawn."))
class UECRPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UECRPawnData(const FObjectInitializer& ObjectInitializer);

public:

	// Class to instantiate for this pawn (should usually derive from AECRPawn or AECRCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECR|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets to grant to this pawn's ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECR|Abilities")
	TArray<UECRAbilitySet*> AbilitySets;

	// What mapping of ability tags to use for actions taking by this pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECR|Abilities")
	UECRAbilityTagRelationshipMapping* TagRelationshipMapping;

	// Input configuration used by player controlled pawns to create input mappings and bind input actions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECR|Input")
	UECRInputConfig* InputConfig;

	// Default camera mode used by player controlled pawns.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ECR|Camera")
	TSubclassOf<UECRCameraMode> DefaultCameraMode;
};
