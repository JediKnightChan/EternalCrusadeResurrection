// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "SimpleActorPoolComponent.generated.h"


USTRUCT()
struct FActorPool
{
	GENERATED_BODY()

	FActorPool();

	UPROPERTY()
	TArray<AActor*> ActorArray;
};

/**
 * Simple storage of several actor pools of different classes, allowing to
 * retrieve an actor from pool or return it there after usage
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SIMPLEACTORPOOLING_API USimpleActorPoolComponent : public UActorComponent
{
	GENERATED_BODY()

	USimpleActorPoolComponent();

protected:
	UFUNCTION(BlueprintCallable)
	AActor* RetrieveActorFromPool(UClass* ActorClass, FTransform SpawnTransform);

	UFUNCTION(BlueprintCallable)
	void ReturnActorToPool(AActor* Actor);

private:
	UPROPERTY()
	TMap<UClass*, FActorPool> PoolMap;
};
