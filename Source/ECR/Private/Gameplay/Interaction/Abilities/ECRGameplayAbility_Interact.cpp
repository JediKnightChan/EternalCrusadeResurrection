// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/Abilities/ECRGameplayAbility_Interact.h"
#include "AbilitySystemComponent.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Interaction/InteractionStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "NativeGameplayTags.h"
#include "GUI/IndicatorSystem/ECRIndicatorManagerComponent.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "GUI/IndicatorSystem/IndicatorDescriptor.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Interaction_Activate, "Ability.Interaction.Activate");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Indicator_Category_Interaction, "GUI.Indicators.Category.Interaction");
UE_DEFINE_GAMEPLAY_TAG(TAG_INTERACTION_DURATION_MESSAGE, "Ability.Interaction.Duration.Message");

UECRGameplayAbility_Interact::UECRGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationPolicy = EECRAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UECRGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                   const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilityActivationInfo ActivationInfo,
                                                   const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
}

void UECRGameplayAbility_Interact::UpdateInteractions(const TArray<FInteractionOption>& InteractiveOptions)
{
	if (AController* PC = GetControllerFromActorInfo())
	{
		if (PC->IsLocalPlayerController())
		{
			if (UECRIndicatorManagerComponent* IndicatorManager = UECRIndicatorManagerComponent::GetComponent(PC))
			{
				for (UIndicatorDescriptor* Indicator : Indicators)
				{
					IndicatorManager->RemoveIndicator(Indicator);
				}
				Indicators.Reset();

				for (const FInteractionOption& InteractionOption : InteractiveOptions)
				{
					AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(
						InteractionOption.InteractableTarget);

					TSoftClassPtr<UUserWidget> InteractionWidgetClass =
						InteractionOption.InteractionWidgetClass.IsNull()
							? DefaultInteractionWidgetClass
							: InteractionOption.InteractionWidgetClass;

					UIndicatorDescriptor* Indicator = NewObject<UIndicatorDescriptor>();
					Indicator->SetCategory(TAG_Indicator_Category_Interaction);
					Indicator->SetDataObject(InteractableTargetActor);
					Indicator->SetSceneComponent(InteractableTargetActor->GetRootComponent());
					Indicator->SetIndicatorClass(InteractionWidgetClass);
					IndicatorManager->AddIndicator(Indicator);

					Indicators.Add(Indicator);
				}
			}
		}
	}

	CurrentOptions = InteractiveOptions;
}

void UECRGameplayAbility_Interact::TriggerInteraction()
{
	if (CurrentOptions.Num() == 0)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	if (AbilitySystem)
	{
		const FInteractionOption& InteractionOption = CurrentOptions[0];

		if (InteractionOption.InputTag.IsValid())
		{
			// Should be activated via input tag, not by same input as interaction
			return;
		}

		AActor* Instigator = GetAvatarActorFromActorInfo();
		AActor* InteractableTargetActor = UInteractionStatics::GetActorFromInteractableTarget(
			InteractionOption.InteractableTarget);

		// Allow the target to customize the event data we're about to pass in, in case the ability needs custom data
		// that only the actor knows.
		FGameplayEventData Payload;
		Payload.EventTag = TAG_Ability_Interaction_Activate;
		Payload.Instigator = Instigator;
		Payload.Target = InteractableTargetActor;

		// If needed we allow the interactable target to manipulate the event data so that for example, a button on the wall
		// may want to specify a door actor to execute the ability on, so it might choose to override Target to be the
		// door actor.
		InteractionOption.InteractableTarget->CustomizeInteractionEventData(TAG_Ability_Interaction_Activate, Payload);

		// Grab the target actor off the payload we're going to use it as the 'avatar' for the interaction, and the
		// source InteractableTarget actor as the owner actor.
		AActor* TargetActor = const_cast<AActor*>(ToRawPtr(Payload.Target));

		// The actor info needed for the interaction.
		FGameplayAbilityActorInfo ActorInfo;
		ActorInfo.InitFromActor(InteractableTargetActor, TargetActor, InteractionOption.TargetAbilitySystem);

		// Trigger the ability using event tag.
		const bool bSuccess = InteractionOption.TargetAbilitySystem->TriggerAbilityFromGameplayEvent(
			InteractionOption.TargetInteractionAbilityHandle,
			&ActorInfo,
			TAG_Ability_Interaction_Activate,
			&Payload,
			*InteractionOption.TargetAbilitySystem
		);
	}
}
