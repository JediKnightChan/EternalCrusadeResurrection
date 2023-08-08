// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "ECREquipmentActor.generated.h"

/**
 * UECREquipmentActor
 *
 * Actor for equipment
 */
UCLASS(BlueprintType, Blueprintable)
class AECREquipmentActor : public AActor
{
	GENERATED_BODY()

public:
	AECREquipmentActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
