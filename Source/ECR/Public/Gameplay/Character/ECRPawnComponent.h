// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "ECRPawnComponent.generated.h"


UINTERFACE(BlueprintType)
class ECR_API UECRReadyInterface : public UInterface
{
	GENERATED_BODY()
};

class IECRReadyInterface
{
	GENERATED_BODY()

public:
	virtual bool IsPawnComponentReadyToInitialize() const = 0;
};




/**
 * UECRPawnComponent
 *
 *	An actor component that can be used for adding custom behavior to pawns.
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class ECR_API UECRPawnComponent : public UPawnComponent, public IECRReadyInterface
{
	GENERATED_BODY()

public:

	UECRPawnComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool IsPawnComponentReadyToInitialize() const override { return true; }
};
