// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "System/ECRLogChannels.h"
#include "System/ECRGameData.h"
#include "System/ECRAssetManager.h"
#include "Gameplay/GAS/ECRGlobalAbilitySystem.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"
#include "Animation/ECRAnimInstance.h"
#include "Gameplay/GAS/ECRAbilityTagRelationshipMapping.h"
#include "Gameplay/ECRGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_AbilityInputBlocked, "Gameplay.AbilityInputBlocked");

UECRAbilitySystemComponent::UECRAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();

	FMemory::Memset(ActivationGroupCounts, 0, sizeof(ActivationGroupCounts));
}

void UECRAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UECRGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UECRGlobalAbilitySystem>(GetWorld()))
	{
		GlobalAbilitySystem->UnregisterASC(this);
	}
}

void UECRAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewPawnAvatar = Cast<APawn>(InAvatarActor) && (InAvatarActor != ActorInfo->AvatarActor);

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bHasNewPawnAvatar)
	{
		// Notify all abilities that a new pawn avatar has been set
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			UECRGameplayAbility* ECRAbilityCDO = CastChecked<UECRGameplayAbility>(AbilitySpec.Ability);

			if (ECRAbilityCDO->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
			{
				TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
				for (UGameplayAbility* AbilityInstance : Instances)
				{
					UECRGameplayAbility* ECRAbilityInstance = CastChecked<UECRGameplayAbility>(AbilityInstance);
					ECRAbilityInstance->OnPawnAvatarSet();
				}
			}
			else
			{
				ECRAbilityCDO->OnPawnAvatarSet();
			}
		}

		// Register with the global system once we actually have a pawn avatar. We wait until this time since some globally-applied effects may require an avatar.
		if (UECRGlobalAbilitySystem* GlobalAbilitySystem = UWorld::GetSubsystem<UECRGlobalAbilitySystem>(GetWorld()))
		{
			GlobalAbilitySystem->RegisterASC(this);
		}

		if (UECRAnimInstance* ECRAnimInst = Cast<UECRAnimInstance>(ActorInfo->GetAnimInstance()))
		{
			ECRAnimInst->InitializeWithAbilitySystem(this);
		}

		TryActivateAbilitiesOnSpawn();
	}
}

void UECRAbilitySystemComponent::TryActivateAbilitiesOnSpawn()
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		const UECRGameplayAbility* ECRAbilityCDO = CastChecked<UECRGameplayAbility>(AbilitySpec.Ability);
		ECRAbilityCDO->TryActivateAbilityOnSpawn(AbilityActorInfo.Get(), AbilitySpec);
	}
}

void UECRAbilitySystemComponent::CancelAbilitiesByFunc(TShouldCancelAbilityFunc ShouldCancelFunc,
                                                       bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();
	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		UECRGameplayAbility* ECRAbilityCDO = CastChecked<UECRGameplayAbility>(AbilitySpec.Ability);

		if (ECRAbilityCDO->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			// Cancel all the spawned instances, not the CDO.
			TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
			for (UGameplayAbility* AbilityInstance : Instances)
			{
				UECRGameplayAbility* ECRAbilityInstance = CastChecked<UECRGameplayAbility>(AbilityInstance);

				if (ShouldCancelFunc(ECRAbilityInstance, AbilitySpec.Handle))
				{
					if (ECRAbilityInstance->CanBeCanceled())
					{
						ECRAbilityInstance->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(),
						                                  ECRAbilityInstance->GetCurrentActivationInfo(),
						                                  bReplicateCancelAbility);
					}
					else
					{
						UE_LOG(LogECRAbilitySystem, Error,
						       TEXT("CancelAbilitiesByFunc: Can't cancel ability [%s] because CanBeCanceled is false."),
						       *ECRAbilityInstance->GetName());
					}
				}
			}
		}
		else
		{
			// Cancel the non-instanced ability CDO.
			if (ShouldCancelFunc(ECRAbilityCDO, AbilitySpec.Handle))
			{
				// Non-instanced abilities can always be canceled.
				check(ECRAbilityCDO->CanBeCanceled());
				ECRAbilityCDO->CancelAbility(AbilitySpec.Handle, AbilityActorInfo.Get(),
				                             FGameplayAbilityActivationInfo(), bReplicateCancelAbility);
			}
		}
	}
}

void UECRAbilitySystemComponent::CancelInputActivatedAbilities(bool bReplicateCancelAbility)
{
	TShouldCancelAbilityFunc ShouldCancelFunc = [this](const UECRGameplayAbility* ECRAbility,
	                                                   FGameplayAbilitySpecHandle Handle)
	{
		const EECRAbilityActivationPolicy ActivationPolicy = ECRAbility->GetActivationPolicy();
		return ((ActivationPolicy == EECRAbilityActivationPolicy::OnInputTriggered) || (ActivationPolicy ==
			EECRAbilityActivationPolicy::WhileInputActive));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UECRAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle,
		                      Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

void UECRAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle,
		                      Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

void UECRAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputPressedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.AddUnique(AbilitySpec.Handle);
			}
		}
	}
}

void UECRAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid())
	{
		for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				InputReleasedSpecHandles.AddUnique(AbilitySpec.Handle);
				InputHeldSpecHandles.Remove(AbilitySpec.Handle);
			}
		}
	}
}

void UECRAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (HasMatchingGameplayTag(TAG_Gameplay_AbilityInputBlocked))
	{
		ClearAbilityInput();
		return;
	}

	static TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reset();

	//@TODO: See if we can use FScopedServerAbilityRPCBatcher ScopedRPCBatcher in some of these loops

	//
	// Process all abilities that activate when the input is held.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability && !AbilitySpec->IsActive())
			{
				const UECRGameplayAbility* ECRAbilityCDO = CastChecked<UECRGameplayAbility>(AbilitySpec->Ability);

				if (ECRAbilityCDO->GetActivationPolicy() == EECRAbilityActivationPolicy::WhileInputActive)
				{
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
				}
			}
		}
	}

	//
	// Process all abilities that had their input pressed this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = true;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputPressed(*AbilitySpec);
				}
				else
				{
					const UECRGameplayAbility* ECRAbilityCDO = CastChecked<UECRGameplayAbility>(AbilitySpec->Ability);

					if (ECRAbilityCDO->GetActivationPolicy() == EECRAbilityActivationPolicy::OnInputTriggered)
					{
						AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
					}
				}
			}
		}
	}

	//
	// Try to activate all the abilities that are from presses and holds.
	// We do it all at once so that held inputs don't activate the ability
	// and then also send a input event to the ability because of the press.
	//
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	//
	// Process all abilities that had their input released this frame.
	//
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AbilitySpec->Ability)
			{
				AbilitySpec->InputPressed = false;

				if (AbilitySpec->IsActive())
				{
					// Ability is active so pass along the input event.
					AbilitySpecInputReleased(*AbilitySpec);
				}
			}
		}
	}

	//
	// Clear the cached ability handles.
	//
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UECRAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UECRAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle,
                                                        UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	UECRGameplayAbility* ECRAbility = CastChecked<UECRGameplayAbility>(Ability);

	AddAbilityToActivationGroup(ECRAbility->GetActivationGroup(), ECRAbility);
}

void UECRAbilitySystemComponent::NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
                                                     const FGameplayTagContainer& FailureReason)
{
	Super::NotifyAbilityFailed(Handle, Ability, FailureReason);

	if (APawn* Avatar = Cast<APawn>(GetAvatarActor()))
	{
		if (!Avatar->IsLocallyControlled() && Ability->IsSupportedForNetworking())
		{
			ClientNotifyAbilityFailed(Ability, FailureReason);
			return;
		}
	}

	HandleAbilityFailed(Ability, FailureReason);
}

void UECRAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
                                                    bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	UECRGameplayAbility* ECRAbility = CastChecked<UECRGameplayAbility>(Ability);

	RemoveAbilityFromActivationGroup(ECRAbility->GetActivationGroup(), ECRAbility);
}

void UECRAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
                                                                UGameplayAbility* RequestingAbility,
                                                                bool bEnableBlockTags,
                                                                const FGameplayTagContainer& BlockTags,
                                                                bool bExecuteCancelTags,
                                                                const FGameplayTagContainer& CancelTags)
{
	FGameplayTagContainer ModifiedBlockTags = BlockTags;
	FGameplayTagContainer ModifiedCancelTags = CancelTags;

	if (TagRelationshipMapping)
	{
		// Use the mapping to expand the ability tags into block and cancel tag
		TagRelationshipMapping->GetAbilityTagsToBlockAndCancel(AbilityTags, &ModifiedBlockTags, &ModifiedCancelTags);
	}

	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags, ModifiedBlockTags,
	                                      bExecuteCancelTags, ModifiedCancelTags);

	//@TODO: Apply any special logic like blocking input or movement
}

void UECRAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags,
                                                                  UGameplayAbility* RequestingAbility,
                                                                  bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);

	//@TODO: Apply any special logic like blocking input or movement
}

void UECRAbilitySystemComponent::GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags,
                                                                        FGameplayTagContainer& OutActivationRequired,
                                                                        FGameplayTagContainer& OutActivationBlocked)
const
{
	if (TagRelationshipMapping)
	{
		TagRelationshipMapping->GetRequiredAndBlockedActivationTags(AbilityTags, &OutActivationRequired,
		                                                            &OutActivationBlocked);
	}
}

void UECRAbilitySystemComponent::ClearAllResettingOnDeathAbilities()
{
	// Get all activatable abilities
	TArray<FGameplayAbilitySpecHandle> OutSpecHandles;
	GetAllAbilities(OutSpecHandles);
	
	for (const FGameplayAbilitySpecHandle CurrentSpecHandle : OutSpecHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(CurrentSpecHandle);
		if (!AbilitySpec->Ability->AbilityTags.HasTag(FECRGameplayTags::Get().Ability_Behavior_SurvivesDeath))
		{
			ClearAbility(AbilitySpec->Handle);
		}
	}
}

void UECRAbilitySystemComponent::SetTagRelationshipMapping(UECRAbilityTagRelationshipMapping* NewMapping)
{
	TagRelationshipMapping = NewMapping;
}

void UECRAbilitySystemComponent::ClientNotifyAbilityFailed_Implementation(
	const UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason)
{
	HandleAbilityFailed(Ability, FailureReason);
}

void UECRAbilitySystemComponent::HandleAbilityFailed(const UGameplayAbility* Ability,
                                                     const FGameplayTagContainer& FailureReason)
{
	//UE_LOG(LogECRAbilitySystem, Warning, TEXT("Ability %s failed to activate (tags: %s)"), *GetPathNameSafe(Ability), *FailureReason.ToString());

	if (const UECRGameplayAbility* ECRAbility = Cast<const UECRGameplayAbility>(Ability))
	{
		ECRAbility->OnAbilityFailedToActivate(FailureReason);
	}
}

bool UECRAbilitySystemComponent::IsActivationGroupBlocked(EECRAbilityActivationGroup Group) const
{
	bool bBlocked = false;

	switch (Group)
	{
	case EECRAbilityActivationGroup::Independent:
		// Independent abilities are never blocked.
		bBlocked = false;
		break;

	case EECRAbilityActivationGroup::Exclusive_Replaceable:
	case EECRAbilityActivationGroup::Exclusive_Blocking:
		// Exclusive abilities can activate if nothing is blocking.
		bBlocked = (ActivationGroupCounts[(uint8)EECRAbilityActivationGroup::Exclusive_Blocking] > 0);
		break;

	default:
		checkf(false, TEXT("IsActivationGroupBlocked: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	return bBlocked;
}

void UECRAbilitySystemComponent::AddAbilityToActivationGroup(EECRAbilityActivationGroup Group,
                                                             UECRGameplayAbility* ECRAbility)
{
	check(ECRAbility);
	check(ActivationGroupCounts[(uint8)Group] < INT32_MAX);

	ActivationGroupCounts[(uint8)Group]++;

	const bool bReplicateCancelAbility = false;

	switch (Group)
	{
	case EECRAbilityActivationGroup::Independent:
		// Independent abilities do not cancel any other abilities.
		break;

	case EECRAbilityActivationGroup::Exclusive_Replaceable:
	case EECRAbilityActivationGroup::Exclusive_Blocking:
		CancelActivationGroupAbilities(EECRAbilityActivationGroup::Exclusive_Replaceable, ECRAbility,
		                               bReplicateCancelAbility);
		break;

	default:
		checkf(false, TEXT("AddAbilityToActivationGroup: Invalid ActivationGroup [%d]\n"), (uint8)Group);
		break;
	}

	const int32 ExclusiveCount = ActivationGroupCounts[(uint8)EECRAbilityActivationGroup::Exclusive_Replaceable] +
		ActivationGroupCounts[(uint8)EECRAbilityActivationGroup::Exclusive_Blocking];
	if (!ensure(ExclusiveCount <= 1))
	{
		UE_LOG(LogECRAbilitySystem, Error,
		       TEXT("AddAbilityToActivationGroup: Multiple exclusive abilities are running."));
	}
}

void UECRAbilitySystemComponent::RemoveAbilityFromActivationGroup(EECRAbilityActivationGroup Group,
                                                                  UECRGameplayAbility* ECRAbility)
{
	check(ECRAbility);
	check(ActivationGroupCounts[(uint8)Group] > 0);

	ActivationGroupCounts[(uint8)Group]--;
}

void UECRAbilitySystemComponent::CancelActivationGroupAbilities(EECRAbilityActivationGroup Group,
                                                                UECRGameplayAbility* IgnoreECRAbility,
                                                                bool bReplicateCancelAbility)
{
	TShouldCancelAbilityFunc ShouldCancelFunc = [this, Group, IgnoreECRAbility](
		const UECRGameplayAbility* ECRAbility, FGameplayAbilitySpecHandle Handle)
	{
		return ((ECRAbility->GetActivationGroup() == Group) && (ECRAbility != IgnoreECRAbility));
	};

	CancelAbilitiesByFunc(ShouldCancelFunc, bReplicateCancelAbility);
}

void UECRAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UECRAssetManager::GetSubclass(
		UECRGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogECRAbilitySystem, Warning,
		       TEXT("AddDynamicTagGameplayEffect: Unable to find DynamicTagGameplayEffect [%s]."),
		       *UECRGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(DynamicTagGE, 1.0f, MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	if (!Spec)
	{
		UE_LOG(LogECRAbilitySystem, Warning,
		       TEXT("AddDynamicTagGameplayEffect: Unable to make outgoing spec for [%s]."), *GetNameSafe(DynamicTagGE));
		return;
	}

	Spec->DynamicGrantedTags.AddTag(Tag);

	ApplyGameplayEffectSpecToSelf(*Spec);
}

void UECRAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
	const TSubclassOf<UGameplayEffect> DynamicTagGE = UECRAssetManager::GetSubclass(
		UECRGameData::Get().DynamicTagGameplayEffect);
	if (!DynamicTagGE)
	{
		UE_LOG(LogECRAbilitySystem, Warning,
		       TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."),
		       *UECRGameData::Get().DynamicTagGameplayEffect.GetAssetName());
		return;
	}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	Query.EffectDefinition = DynamicTagGE;

	RemoveActiveEffects(Query);
}

void UECRAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle,
                                                      FGameplayAbilityActivationInfo ActivationInfo,
                                                      FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
	TSharedPtr<FAbilityReplicatedDataCache> ReplicatedData = AbilityTargetDataMap.Find(
		FGameplayAbilitySpecHandleAndPredictionKey(AbilityHandle, ActivationInfo.GetActivationPredictionKey()));
	if (ReplicatedData.IsValid())
	{
		OutTargetDataHandle = ReplicatedData->TargetData;
	}
}
