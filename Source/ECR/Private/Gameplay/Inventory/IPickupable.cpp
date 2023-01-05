// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Inventory/IPickupable.h"
#include "DrawDebugHelpers.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "UObject/ScriptInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Gameplay/Inventory/ECRInventoryManagerComponent.h"

UPickupableStatics::UPickupableStatics()
	: Super(FObjectInitializer::Get())
{
}

TScriptInterface<IPickupable> UPickupableStatics::GetIPickupableFromActorInfo(UGameplayAbility* Ability)
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	AActor* Avatar = ActorInfo->AvatarActor.Get();

	// If the actor is directly pickupable, return that.
	TScriptInterface<IPickupable> PickupableActor(Avatar);
	if (PickupableActor)
	{
		return PickupableActor;
	}

	// If the actor isn't pickupable, it might have a component that has a pickupable interface.
	TArray<UActorComponent*> PickupableComponents = Avatar ? Avatar->GetComponentsByInterface(UPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		ensureMsgf(PickupableComponents.Num() == 1, TEXT("We don't support multiple pickupable components yet."));

		return TScriptInterface<IPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IPickupable>();
}

void UPickupableStatics::AddPickupInventory(UECRInventoryManagerComponent* InventoryComponent, TScriptInterface<IPickupable> Pickupable)
{
	if (InventoryComponent && Pickupable)
	{
		const FInventoryPickup& PickupInventory = Pickupable->GetPickupInventory();

		for (const FPickupTemplate& Template : PickupInventory.Templates)
		{
			InventoryComponent->AddItemDefinition(Template.ItemDef, Template.StackCount);
		}

		for (const FPickupInstance& Instance : PickupInventory.Instances)
		{
			InventoryComponent->AddItemInstance(Instance.Item);
		}
	}
}