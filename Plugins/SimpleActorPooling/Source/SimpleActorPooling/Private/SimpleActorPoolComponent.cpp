#include "SimpleActorPoolComponent.h"

#include "IPoolableActor.h"

FActorPool::FActorPool()
{
	ActorArray = {};
}

USimpleActorPoolComponent::USimpleActorPoolComponent()
{
}

AActor* USimpleActorPoolComponent::RetrieveActorFromPool(UClass* ActorClass, FTransform SpawnTransform, bool bLogDebug)
{
	// First try to return a free actor from pool
	if (FActorPool* Pool = PoolMap.Find(ActorClass))
	{
		if (!Pool->ActorArray.IsEmpty())
		{
			AActor* Actor = Pool->ActorArray.Pop();
			if (Actor)
			{
				if (bLogDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("SimplePool: retrieved actor from pool"))
				}
				Actor->SetActorTransform(SpawnTransform);
				IPoolableActor::Execute_OnSpawnedFromPool(Actor, false);
				return Actor;
			}
		}
	}

	// Didn't find free actor in pool, need to spawn a new one
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorClass, Params);
		if (bLogDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("SimplePool: had to spawn new actor"))
		}
		if (SpawnedActor)
		{
			SpawnedActor->SetActorTransform(SpawnTransform);
			IPoolableActor::Execute_OnSpawnedFromPool(SpawnedActor, true);

			if (!PoolMap.Contains(ActorClass))
			{
				PoolMap.Add(ActorClass, {});
			}

			return SpawnedActor;
		} else
		{
			UE_LOG(LogTemp, Error, TEXT("SimplePool: SPAWNED ACTOR NULL"))
		}
	} else
	{
		if (bLogDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("SimplePool: WORLD NULL"))
		}
	}

	return nullptr;
}

void USimpleActorPoolComponent::ReturnActorToPool(AActor* Actor, bool bLogDebug)
{
	if (!Actor)
	{
		return;
	}

	UClass* ActorClass = Actor->GetClass();

	// Only return to pool if pool map already knows about this class
	if (FActorPool* Pool = PoolMap.Find(ActorClass))
	{
		if (bLogDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("SimplePool: returned actor to pool"))
		}

		Pool->ActorArray.AddUnique(Actor);
		IPoolableActor::Execute_OnReturnedToPool(Actor);
	}
	else
	{
		if (bLogDebug)
		{
			UE_LOG(LogTemp, Error, TEXT("SimplePool: RETURN FAILED: NO KEY!"))
		}
	}
}
