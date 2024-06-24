#include "SimpleActorPoolComponent.h"

#include "IPoolableActor.h"

FActorPool::FActorPool()
{
	ActorArray = {};
}

USimpleActorPoolComponent::USimpleActorPoolComponent()
{
}

AActor* USimpleActorPoolComponent::RetrieveActorFromPool(UClass* ActorClass, FTransform SpawnTransform)
{
	// First try to return a free actor from pool
	if (FActorPool* Pool = PoolMap.Find(ActorClass))
	{
		if (!Pool->ActorArray.IsEmpty())
		{
			AActor* Actor = Pool->ActorArray.Pop();
			if (Actor)
			{
				Actor->SetActorTransform(SpawnTransform);

				if (IPoolableActor* PoolableActor = Cast<IPoolableActor>(Actor))
				{
					PoolableActor->OnSpawnedFromPool(false);
				}

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

		if (SpawnedActor)
		{
			SpawnedActor->SetActorTransform(SpawnTransform);

			if (IPoolableActor* PoolableSpawnedActor = Cast<IPoolableActor>(SpawnedActor))
			{
				PoolableSpawnedActor->OnSpawnedFromPool(true);
			}

			if (!PoolMap.Contains(ActorClass))
			{
				PoolMap.Add(ActorClass, {});
			}
		}
	}

	return nullptr;
}

void USimpleActorPoolComponent::ReturnActorToPool(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	UClass* ActorClass = Actor->GetClass();

	// Only return to pool if pool map already knows about this class
	if (FActorPool* Pool = PoolMap.Find(ActorClass))
	{
		Pool->ActorArray.AddUnique(Actor);
		if (IPoolableActor* PoolableActor = Cast<IPoolableActor>(Actor))
		{
			PoolableActor->OnReturnedToPool();
		}
	}
}
