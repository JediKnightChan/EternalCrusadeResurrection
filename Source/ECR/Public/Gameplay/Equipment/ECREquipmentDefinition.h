// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Cosmetics/ECRCosmeticAnimationTypes.h"

#include "ECREquipmentDefinition.generated.h"

class UECRAbilitySet;
class UECREquipmentInstance;

USTRUCT(BlueprintType)
struct FECREquipmentActorToSpawn
{
	GENERATED_BODY()

	FECREquipmentActorToSpawn()
	{
	}

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	FECRActorSelectionSet ActorSelectionSet;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};


/**
 * UECREquipmentDefinition
 *
 * Definition of a piece of equipment that can be applied to a pawn
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UECREquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UECREquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Class to spawn
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	TSubclassOf<UECREquipmentInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	TArray<TObjectPtr<const UECRAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category=Equipment)
	TArray<FECREquipmentActorToSpawn> ActorsToSpawn;
};
