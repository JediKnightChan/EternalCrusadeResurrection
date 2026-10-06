#include "ECREquipmentActor.h"

AECREquipmentActor::AECREquipmentActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AECREquipmentActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!bDisableChildComponentsTickOnDedicatedServer)
	{
		return;
	}

	if (GetNetMode() == NM_DedicatedServer)
	{
		TArray<UActorComponent*> Components;
		GetComponents(Components);

		for (UActorComponent* Comp : Components)
		{
			if (Comp && Comp->PrimaryComponentTick.bCanEverTick)
			{
				Comp->PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
			}
		}
	}
}
