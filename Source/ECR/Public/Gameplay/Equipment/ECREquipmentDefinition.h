// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Cosmetics/ECRCosmeticAnimationTypes.h"

#include "ECREquipmentDefinition.generated.h"

class UECRAbilitySet;
class UECREquipmentInstance;

USTRUCT()
struct FECREquipmentActorToSpawn
{
	GENERATED_BODY()

	FECREquipmentActorToSpawn()
	{
	}

	UPROPERTY(EditAnywhere, Category=Equipment)
	FECRActorSelectionSet ActorSelectionSet;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment)
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
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<UECREquipmentInstance> InstanceType;

	// Gameplay ability sets to grant when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UECRAbilitySet>> AbilitySetsToGrant;

	// Actors to spawn on the pawn when this is equipped
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FECREquipmentActorToSpawn> ActorsToSpawn;
};
