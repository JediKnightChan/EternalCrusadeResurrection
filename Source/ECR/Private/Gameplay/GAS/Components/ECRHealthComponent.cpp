// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Components/ECRHealthComponent.h"
#include "System/ECRLogChannels.h"
#include "System/ECRAssetManager.h"
#include "System/ECRGameData.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayPrediction.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "System/Messages/ECRVerbMessage.h"
#include "System/Messages/ECRVerbMessageHelpers.h"
#include "NativeGameplayTags.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Elimination_Message, "ECR.Elimination.Message");


UECRHealthComponent::UECRHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	AbilitySystemComponent = nullptr;
	HealthSet = nullptr;
	DeathState = EECRDeathState::NotDead;
}

void UECRHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UECRHealthComponent, DeathState);
}

void UECRHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();

	Super::OnUnregister();
}

float UECRHealthComponent::GetNormalizedAttributeValue(const float Value, const float MaxValue)
{
	return ((MaxValue > 0.0f) ? (Value / MaxValue) : 0.0f);
}


void UECRHealthComponent::InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogECR, Error,
		       TEXT(
			       "ECRHealthComponent: Health component for owner [%s] has already been initialized with an ability system."
		       ), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InASC;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogECR, Error,
		       TEXT("ECRHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."),
		       *GetNameSafe(Owner));
		return;
	}

	ClearGameplayTags();

	HealthSet = AbilitySystemComponent->GetSet<UECRHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogECR, Error,
		       TEXT(
			       "ECRHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."
		       ), *GetNameSafe(Owner));
		return;
	}

	// Register to listen for attribute changes.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRHealthSet::GetHealthAttribute()).AddUObject(
		this, &ThisClass::HandleHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRHealthSet::GetMaxHealthAttribute()).AddUObject(
		this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnReadyToDie.AddUObject(this, &ThisClass::HandleReadyToDie);

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);

	//UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(GetOwner(), UGameFrameworkComponentManager::NAME_HealthComponentReady);
}

void UECRHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnReadyToDie.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UECRHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();

		AbilitySystemComponent->SetLooseGameplayTagCount(GameplayTags.Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(GameplayTags.Status_Death_Dead, 0);
	}
}

float UECRHealthComponent::GetHealth() const
{
	return (HealthSet ? HealthSet->GetHealth() : 0.0f);
}

float UECRHealthComponent::GetMaxHealth() const
{
	return (HealthSet ? HealthSet->GetMaxHealth() : 0.0f);
}

float UECRHealthComponent::GetHealthNormalized() const
{
	return GetNormalizedAttributeValue(GetHealth(), GetMaxHealth());
}

AActor* UECRHealthComponent::GetInstigatorFromAttrChangeData(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.GEModData != nullptr)
	{
		const FGameplayEffectContextHandle& EffectContext = ChangeData.GEModData->EffectSpec.GetEffectContext();
		return EffectContext.GetOriginalInstigator();
	}

	return nullptr;
}

void UECRHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                          GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                             GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRHealthComponent::HandleReadyToDie(AActor* DamageInstigator, AActor* DamageCauser,
                                           const FGameplayEffectSpec& DamageEffectSpec, float DamageMagnitude)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent)
	{
		// Send the "GameplayEvent.Death" gameplay event through the owner's ability system.  This can be used to trigger a death gameplay ability.
		{
			FGameplayEventData Payload;
			Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_Death;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec.Def;
			Payload.ContextHandle = DamageEffectSpec.GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec.CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec.CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		// Send a standardized verb message that other systems can observe
		{
			FECRVerbMessage Message;
			Message.Verb = TAG_ECR_Elimination_Message;
			Message.Instigator = DamageInstigator;
			Message.InstigatorTags = *DamageEffectSpec.CapturedSourceTags.GetAggregatedTags();
			Message.Target = UECRVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
			Message.TargetTags = *DamageEffectSpec.CapturedTargetTags.GetAggregatedTags();
			//@TODO: Fill out context tags, and any non-ability-system source/instigator tags
			//@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}

		//@TODO: assist messages (could compute from damage dealt elsewhere)?
	}

#endif // #if WITH_SERVER_CODE
}

void UECRHealthComponent::OnRep_DeathState(EECRDeathState OldDeathState)
{
	const EECRDeathState NewDeathState = DeathState;

	// Revert the death state for now since we rely on StartDeath and FinishDeath to change it.
	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		// The server is trying to set us back but we've already predicted past the server state.
		UE_LOG(LogECR, Warning,
		       TEXT("ECRHealthComponent: Predicted past server death state [%d] -> [%d] for owner [%s]."),
		       (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EECRDeathState::NotDead)
	{
		if (NewDeathState == EECRDeathState::DeathStarted)
		{
			StartDeath();
		}
		else if (NewDeathState == EECRDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogECR, Error, TEXT("ECRHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."),
			       (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}
	else if (OldDeathState == EECRDeathState::DeathStarted)
	{
		if (NewDeathState == EECRDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogECR, Error, TEXT("ECRHealthComponent: Invalid death transition [%d] -> [%d] for owner [%s]."),
			       (uint8)OldDeathState, (uint8)NewDeathState, *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf((DeathState == NewDeathState),
	           TEXT("ECRHealthComponent: Death transition failed [%d] -> [%d] for owner [%s]."), (uint8)OldDeathState,
	           (uint8)NewDeathState, *GetNameSafe(GetOwner()));
}

void UECRHealthComponent::StartDeath()
{
	if (DeathState != EECRDeathState::NotDead)
	{
		return;
	}

	DeathState = EECRDeathState::DeathStarted;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathStarted.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

void UECRHealthComponent::FinishDeath()
{
	if (DeathState != EECRDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EECRDeathState::DeathFinished;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);

	OnDeathFinished.Broadcast(Owner);

	Owner->ForceNetUpdate();
}

float UECRHealthComponent::GetDamageToKill()
{
	return GetMaxHealth();
}

void UECRHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	if ((DeathState == EECRDeathState::NotDead) && AbilitySystemComponent)
	{
		const TSubclassOf<UGameplayEffect> DamageGE = UECRAssetManager::GetSubclass(
			UECRGameData::Get().DamageGameplayEffect_SetByCaller);
		if (!DamageGE)
		{
			UE_LOG(LogECR, Error,
			       TEXT(
				       "ECRHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to find gameplay effect [%s]."
			       ), *GetNameSafe(GetOwner()), *UECRGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			DamageGE, 1.0f, AbilitySystemComponent->MakeEffectContext());
		FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

		if (!Spec)
		{
			UE_LOG(LogECR, Error,
			       TEXT(
				       "ECRHealthComponent: DamageSelfDestruct failed for owner [%s]. Unable to make outgoing spec for [%s]."
			       ), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGE));
			return;
		}

		Spec->AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);

		if (bFellOutOfWorld)
		{
			Spec->AddDynamicAssetTag(TAG_Gameplay_FellOutOfWorld);
		}

		const float DamageAmount = GetDamageToKill();

		Spec->SetSetByCallerMagnitude(FECRGameplayTags::Get().SetByCaller_Damage, DamageAmount);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
	}
}
