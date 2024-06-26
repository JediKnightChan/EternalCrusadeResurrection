// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPoolableActor.generated.h"

/**  */
UINTERFACE(BlueprintType)
class UPoolableActor : public UInterface
{
	GENERATED_BODY()
};

/**  */
class IPoolableActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnSpawnedFromPool(bool bFirstTimeSpawned = false);

	UFUNCTION(BlueprintImplementableEvent)
	void OnReturnedToPool();
};
