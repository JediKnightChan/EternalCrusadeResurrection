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
 *
 * Need to store array on something that isn't destroyed, Player Controller for map (recommended)
 * or GameInstance for the whole game
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class SIMPLEACTORPOOLING_API USimpleActorPoolComponent : public UActorComponent
{
	GENERATED_BODY()

	USimpleActorPoolComponent();

protected:
	UFUNCTION(BlueprintCallable)
	AActor* RetrieveActorFromPool(UClass* ActorClass, FTransform SpawnTransform, bool bLogDebug = false);

	UFUNCTION(BlueprintCallable)
	void ReturnActorToPool(AActor* Actor, bool bLogDebug = false);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE TArray<UClass*> GetPoolMapCurrentKeys() const
	{
		TArray<UClass*> Out;
		PoolMap.GetKeys(Out);
		return Out;
	}

	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetPoolMapCurrentValueSize(UClass* Class) const
	{
		if (const FActorPool* ActorPool = PoolMap.Find(Class))
		{
			return ActorPool->ActorArray.Num();
		}
		return -1;
	}
private:
	UPROPERTY()
	TMap<UClass*, FActorPool> PoolMap;
};
