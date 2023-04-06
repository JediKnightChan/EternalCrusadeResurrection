// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/GAS/Abilities/ECRAbilityCost.h"
#include "Gameplay/Character/ECRHeroComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "Cosmetics/ECRCosmeticStatics.h"
#include "Cosmetics/ECRPawnComponent_CharacterParts.h"
#include "Engine/AssetManager.h"
#include "Gameplay/GAS/Abilities/ECRAbilitySimpleFailureMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/Player/ECRPlayerState.h"
#include "Physics/PhysicalMaterialWithTags.h"


#define ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(FunctionName, ReturnValue)																				\
{																																						\
if (!ensure(IsInstantiated()))																														\
{																																					\
ABILITY_LOG(Error, TEXT("%s: " #FunctionName " cannot be called on a non-instanced ability. Check the instancing policy."), *GetPathName());	\
return ReturnValue;																																\
}																																					\
}

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_SIMPLE_FAILURE_MESSAGE, "Ability.UserFacingSimpleActivateFail.Message");
UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_PLAY_MONTAGE_FAILURE_MESSAGE, "Ability.PlayMontageOnActivateFail.Message");

UECRGameplayAbility::UECRGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EECRAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EECRAbilityActivationGroup::Independent;

	ActiveCameraMode = nullptr;
}

UECRAbilitySystemComponent* UECRGameplayAbility::GetECRAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo
		        ? Cast<UECRAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get())
		        : nullptr);
}

AECRPlayerController* UECRGameplayAbility::GetECRPlayerControllerFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AECRPlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr);
}

AController* UECRGameplayAbility::GetControllerFromActorInfo() const
{
	if (CurrentActorInfo)
	{
		if (AController* PC = CurrentActorInfo->PlayerController.Get())
		{
			return PC;
		}

		// Look for a player controller or pawn in the owner chain.
		AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
		while (TestActor)
		{
			if (AController* C = Cast<AController>(TestActor))
			{
				return C;
			}

			if (APawn* Pawn = Cast<APawn>(TestActor))
			{
				return Pawn->GetController();
			}

			TestActor = TestActor->GetOwner();
		}
	}

	return nullptr;
}

AECRCharacter* UECRGameplayAbility::GetECRCharacterFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<AECRCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

UECRHeroComponent* UECRGameplayAbility::GetHeroComponentFromActorInfo() const
{
	return (CurrentActorInfo ? UECRHeroComponent::FindHeroComponent(CurrentActorInfo->AvatarActor.Get()) : nullptr);
}

void UECRGameplayAbility::ToggleInputDisabled(const bool NewInputDisabled)
{
	AECRPlayerController* PC = GetECRPlayerControllerFromActorInfo();
	AECRCharacter* Character = GetECRCharacterFromActorInfo();
	if (PC && Character)
	{
		if (NewInputDisabled)
		{
			Character->DisableInput(PC);
		}
		else
		{
			Character->EnableInput(PC);
		}
	}
}

void UECRGameplayAbility::ToggleMovementEnabled(const bool bNewEnabled)
{
	if (UECRHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
	{
		HeroComponent->ToggleMovementInput(bNewEnabled);
	}
}

void UECRGameplayAbility::NativeOnAbilityFailedToActivate(const FGameplayTagContainer& FailedReason) const
{
	bool bSimpleFailureFound = false;
	for (FGameplayTag Reason : FailedReason)
	{
		if (!bSimpleFailureFound)
		{
			if (const FText* pUserFacingMessage = FailureTagToUserFacingMessages.Find(Reason))
			{
				FECRAbilitySimpleFailureMessage Message;
				Message.PlayerController = GetActorInfo().PlayerController.Get();
				Message.FailureTags = FailedReason;
				Message.UserFacingReason = *pUserFacingMessage;

				UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
				MessageSystem.BroadcastMessage(TAG_ABILITY_SIMPLE_FAILURE_MESSAGE, Message);
				bSimpleFailureFound = true;
			}
		}

		if (UAnimMontage* pMontage = FailureTagToAnimMontage.FindRef(Reason))
		{
			FECRAbilityMontageFailureMessage Message;
			Message.PlayerController = GetActorInfo().PlayerController.Get();
			Message.FailureTags = FailedReason;
			Message.FailureMontage = pMontage;

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(TAG_ABILITY_PLAY_MONTAGE_FAILURE_MESSAGE, Message);
		}
	}
}

bool UECRGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                             const FGameplayAbilityActorInfo* ActorInfo,
                                             const FGameplayTagContainer* SourceTags,
                                             const FGameplayTagContainer* TargetTags,
                                             FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	UECRAbilitySystemComponent* ECRASC = CastChecked<UECRAbilitySystemComponent>(
		ActorInfo->AbilitySystemComponent.Get());
	const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	//@TODO Possibly remove after setting up tag relationships
	if (ECRASC->IsActivationGroupBlocked(ActivationGroup))
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(GameplayTags.Ability_ActivateFail_ActivationGroup);
		}
		return false;
	}

	return true;
}

void UECRGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	// The ability can not block canceling if it's replaceable.
	if (!bCanBeCanceled && (ActivationGroup == EECRAbilityActivationGroup::Exclusive_Replaceable))
	{
		UE_LOG(LogECRAbilitySystem, Error,
		       TEXT(
			       "SetCanBeCanceled: Ability [%s] can not block canceling because its activation group is replaceable."
		       ), *GetName());
		return;
	}

	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UECRGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	LoadMontages();


	if (UECRPawnComponent_CharacterParts* CosmeticComp = GetPawnCustomizationComponent())
	{
		CosmeticComp->OnCharacterPartsChanged.AddDynamic(this, &ThisClass::OnCharacterPartsChanged);
	}


	K2_OnAbilityAdded();

	TryActivateAbilityOnSpawn(ActorInfo, Spec);
}

void UECRGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	K2_OnAbilityRemoved();

	Super::OnRemoveAbility(ActorInfo, Spec);

	if (UECRPawnComponent_CharacterParts* CosmeticComp = GetPawnCustomizationComponent())
	{
		CosmeticComp->OnCharacterPartsChanged.RemoveDynamic(this, &ThisClass::OnCharacterPartsChanged);
	}
}

void UECRGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UECRGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                     bool bWasCancelled)
{
	ClearCameraMode();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UECRGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) || !ActorInfo)
	{
		return false;
	}

	// Verify we can afford any additional costs
	for (TObjectPtr<UECRAbilityCost> AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (!AdditionalCost->CheckCost(this, Handle, ActorInfo, /*inout*/ OptionalRelevantTags))
			{
				return false;
			}
		}
	}

	return true;
}

void UECRGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	check(ActorInfo);

	// Used to determine if the ability actually hit a target (as some costs are only spent on successful attempts)
	auto DetermineIfAbilityHitTarget = [&]()
	{
		if (ActorInfo->IsNetAuthority())
		{
			if (UECRAbilitySystemComponent* ASC = Cast<UECRAbilitySystemComponent>(
				ActorInfo->AbilitySystemComponent.Get()))
			{
				FGameplayAbilityTargetDataHandle TargetData;
				ASC->GetAbilityTargetData(Handle, ActivationInfo, TargetData);
				for (int32 TargetDataIdx = 0; TargetDataIdx < TargetData.Data.Num(); ++TargetDataIdx)
				{
					if (UAbilitySystemBlueprintLibrary::TargetDataHasHitResult(TargetData, TargetDataIdx))
					{
						return true;
					}
				}
			}
		}

		return false;
	};

	// Pay any additional costs
	bool bAbilityHitTarget = false;
	bool bHasDeterminedIfAbilityHitTarget = false;
	for (TObjectPtr<UECRAbilityCost> AdditionalCost : AdditionalCosts)
	{
		if (AdditionalCost != nullptr)
		{
			if (AdditionalCost->ShouldOnlyApplyCostOnHit())
			{
				if (!bHasDeterminedIfAbilityHitTarget)
				{
					bAbilityHitTarget = DetermineIfAbilityHitTarget();
					bHasDeterminedIfAbilityHitTarget = true;
				}

				if (!bAbilityHitTarget)
				{
					continue;
				}
			}

			AdditionalCost->ApplyCost(this, Handle, ActorInfo, ActivationInfo);
		}
		else
		{
			UE_LOG(LogECRAbilitySystem, Warning, TEXT("Additional cost is nullptr"))
		}
	}
}

FGameplayEffectContextHandle UECRGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle,
                                                                    const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

	FECRGameplayEffectContext* EffectContext = FECRGameplayEffectContext::ExtractEffectContext(ContextHandle);
	check(EffectContext);

	check(ActorInfo);

	AActor* EffectCauser = nullptr;
	const IECRAbilitySourceInterface* AbilitySource = nullptr;
	float SourceLevel = 0.0f;
	GetAbilitySource(Handle, ActorInfo, /*out*/ SourceLevel, /*out*/ AbilitySource, /*out*/ EffectCauser);

	UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

	AActor* Instigator = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;

	EffectContext->SetAbilitySource(AbilitySource, SourceLevel);
	EffectContext->AddInstigator(Instigator, EffectCauser);
	EffectContext->AddSourceObject(SourceObject);

	return ContextHandle;
}

void UECRGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec(FGameplayEffectSpec& Spec,
                                                               FGameplayAbilitySpec* AbilitySpec) const
{
	Super::ApplyAbilityTagsToGameplayEffectSpec(Spec, AbilitySpec);

	if (const FHitResult* HitResult = Spec.GetContext().GetHitResult())
	{
		if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(
			HitResult->PhysMaterial.Get()))
		{
			Spec.CapturedTargetTags.GetSpecTags().AppendTags(PhysMatWithTags->Tags);
		}
	}
}

bool UECRGameplayAbility::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent,
                                                            const FGameplayTagContainer* SourceTags,
                                                            const FGameplayTagContainer* TargetTags,
                                                            OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	// Specialized version to handle death exclusion and AbilityTags expansion via ASC

	bool bBlocked = false;
	bool bMissing = false;

	UAbilitySystemGlobals& AbilitySystemGlobals = UAbilitySystemGlobals::Get();
	const FGameplayTag& BlockedTag = AbilitySystemGlobals.ActivateFailTagsBlockedTag;
	const FGameplayTag& MissingTag = AbilitySystemGlobals.ActivateFailTagsMissingTag;

	// Check if any of this ability's tags are currently blocked
	if (AbilitySystemComponent.AreAbilityTagsBlocked(AbilityTags))
	{
		bBlocked = true;
	}

	const UECRAbilitySystemComponent* ECRASC = Cast<UECRAbilitySystemComponent>(&AbilitySystemComponent);
	static FGameplayTagContainer AllRequiredTags;
	static FGameplayTagContainer AllBlockedTags;

	AllRequiredTags = ActivationRequiredTags;
	AllBlockedTags = ActivationBlockedTags;

	// Expand our ability tags to add additional required/blocked tags
	if (ECRASC)
	{
		ECRASC->GetAdditionalActivationTagRequirements(AbilityTags, AllRequiredTags, AllBlockedTags);
	}

	// Check to see the required/blocked tags for this ability
	if (AllBlockedTags.Num() || AllRequiredTags.Num())
	{
		static FGameplayTagContainer AbilitySystemComponentTags;

		AbilitySystemComponentTags.Reset();
		AbilitySystemComponent.GetOwnedGameplayTags(AbilitySystemComponentTags);

		if (AbilitySystemComponentTags.HasAny(AllBlockedTags))
		{
			const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();
			if (OptionalRelevantTags && AbilitySystemComponentTags.HasTag(GameplayTags.Status_Death))
			{
				// If player is dead and was rejected due to blocking tags, give that feedback
				OptionalRelevantTags->AddTag(GameplayTags.Ability_ActivateFail_IsDead);
			}

			bBlocked = true;
		}

		if (!AbilitySystemComponentTags.HasAll(AllRequiredTags))
		{
			bMissing = true;
		}
	}

	if (SourceTags != nullptr)
	{
		if (SourceBlockedTags.Num() || SourceRequiredTags.Num())
		{
			if (SourceTags->HasAny(SourceBlockedTags))
			{
				bBlocked = true;
			}

			if (!SourceTags->HasAll(SourceRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (TargetTags != nullptr)
	{
		if (TargetBlockedTags.Num() || TargetRequiredTags.Num())
		{
			if (TargetTags->HasAny(TargetBlockedTags))
			{
				bBlocked = true;
			}

			if (!TargetTags->HasAll(TargetRequiredTags))
			{
				bMissing = true;
			}
		}
	}

	if (bBlocked)
	{
		if (OptionalRelevantTags && BlockedTag.IsValid())
		{
			OptionalRelevantTags->AddTag(BlockedTag);
		}
		return false;
	}
	if (bMissing)
	{
		if (OptionalRelevantTags && MissingTag.IsValid())
		{
			OptionalRelevantTags->AddTag(MissingTag);
		}
		return false;
	}

	return true;
}

void UECRGameplayAbility::OnPawnAvatarSet()
{
	K2_OnPawnAvatarSet();
}

void UECRGameplayAbility::GetAbilitySource(FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel,
                                           const IECRAbilitySourceInterface*& OutAbilitySource,
                                           AActor*& OutEffectCauser) const
{
	OutSourceLevel = 0.0f;
	OutAbilitySource = nullptr;
	OutEffectCauser = nullptr;

	OutEffectCauser = ActorInfo->AvatarActor.Get();

	// If we were added by something that's an ability info source, use it
	UObject* SourceObject = GetSourceObject(Handle, ActorInfo);

	OutAbilitySource = Cast<IECRAbilitySourceInterface>(SourceObject);
}


UAnimMontage* UECRGameplayAbility::GetMontage(const FName MontageCategory) const
{
	const FECRAnimMontageSelectionSet AnimMontageSelectionSet = AbilityMontageSelection.FindRef(MontageCategory);
	if (const UECRPawnComponent_CharacterParts* CustomizationComponent =
		GetPawnCustomizationComponent())
	{
		const FGameplayTagContainer MontageTags = CustomizationComponent->GetCombinedTags(
			FECRGameplayTags::Get().Cosmetic_Montage);
		const TSoftObjectPtr<UAnimMontage> AnimMontage = AnimMontageSelectionSet.SelectBestMontage(MontageTags);
		if (!AnimMontage.IsValid())
		{
			if (!AnimMontage.IsNull())
			{
				UAssetManager::GetStreamableManager().RequestSyncLoad(AnimMontage.ToSoftObjectPath());
				UE_LOG(LogECR, Warning, TEXT("Had to sync load ability montage %s!"), *(AnimMontage.GetAssetName()))
			}
		}
		return AnimMontage.Get();
	}
	return nullptr;
}

void UECRGameplayAbility::OnCharacterPartsChanged(UECRPawnComponent_CharacterParts* ComponentWithChangedParts)
{
	LoadMontages();
}

UECRPawnComponent_CharacterParts* UECRGameplayAbility::GetPawnCustomizationComponent() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	if (const AECRPlayerState* PS = Cast<AECRPlayerState>(CurrentActorInfo->OwnerActor))
	{
		if (const APawn* Pawn = PS->GetPawn())
		{
			return Pawn->FindComponentByClass<UECRPawnComponent_CharacterParts>();
		}
		return nullptr;
	}
	return nullptr;
}


void UECRGameplayAbility::LoadMontages()
{
	TArray<FSoftObjectPath> MontagesToLoad;

	if (const UECRPawnComponent_CharacterParts* CustomizationComponent = GetPawnCustomizationComponent())
	{
		for (const auto& [Name, SelectionSet] : AbilityMontageSelection)
		{
			FGameplayTagContainer GameplayTags = CustomizationComponent->GetCombinedTags(
				FECRGameplayTags::Get().Cosmetic_Montage);
			const TSoftObjectPtr<UAnimMontage> AnimMontage = SelectionSet.SelectBestMontage(GameplayTags);
			UECRCosmeticStatics::AddMontageToLoadQueueIfNeeded(AnimMontage, MontagesToLoad);
		}
	}

	if (MontagesToLoad.Num())
	{
		if (UAssetManager::IsValid())
		{
			UAssetManager::GetStreamableManager().RequestAsyncLoad(MontagesToLoad);
		}
	}
}


void UECRGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
                                                    const FGameplayAbilitySpec& Spec) const
{
	const bool bIsPredicting = (Spec.ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	// Try to activate if activation policy is on spawn.
	if (ActorInfo && !Spec.IsActive() && !bIsPredicting && (ActivationPolicy == EECRAbilityActivationPolicy::OnSpawn))
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

		// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
		if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
		{
			const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) ||
				(NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
			const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (
				NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

			const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
			const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

			if (bClientShouldActivate || bServerShouldActivate)
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}
}

bool UECRGameplayAbility::CanChangeActivationGroup(EECRAbilityActivationGroup NewGroup) const
{
	if (!IsInstantiated() || !IsActive())
	{
		return false;
	}

	if (ActivationGroup == NewGroup)
	{
		return true;
	}

	UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponentFromActorInfo();
	check(ECRASC);

	if ((ActivationGroup != EECRAbilityActivationGroup::Exclusive_Blocking) && ECRASC->
		IsActivationGroupBlocked(NewGroup))
	{
		// This ability can't change groups if it's blocked (unless it is the one doing the blocking).
		return false;
	}

	if ((NewGroup == EECRAbilityActivationGroup::Exclusive_Replaceable) && !CanBeCanceled())
	{
		// This ability can't become replaceable if it can't be canceled.
		return false;
	}

	return true;
}

bool UECRGameplayAbility::ChangeActivationGroup(EECRAbilityActivationGroup NewGroup)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ChangeActivationGroup, false);

	if (!CanChangeActivationGroup(NewGroup))
	{
		return false;
	}

	if (ActivationGroup != NewGroup)
	{
		UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponentFromActorInfo();
		check(ECRASC);

		ECRASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
		ECRASC->AddAbilityToActivationGroup(NewGroup, this);

		ActivationGroup = NewGroup;
	}

	return true;
}

void UECRGameplayAbility::SetCameraMode(TSubclassOf<UECRCameraMode> CameraMode)
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(SetCameraMode,);

	if (UECRHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
	{
		HeroComponent->SetAbilityCameraMode(CameraMode, CurrentSpecHandle);
		ActiveCameraMode = CameraMode;
	}
}

void UECRGameplayAbility::ClearCameraMode()
{
	ENSURE_ABILITY_IS_INSTANTIATED_OR_RETURN(ClearCameraMode,);

	if (ActiveCameraMode)
	{
		if (UECRHeroComponent* HeroComponent = GetHeroComponentFromActorInfo())
		{
			HeroComponent->ClearAbilityCameraMode(CurrentSpecHandle);
		}

		ActiveCameraMode = nullptr;
	}
}
