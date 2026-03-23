// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Character/ECRCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Gameplay/Character/ECRCharacterMovementComponent.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "System/ECRSignificanceManager.h"
#include "Components/InputComponent.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Gameplay/Player/ECRPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/ActorChannel.h"
#include "Gameplay/ECRGameState.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Gameplay/GAS/Attributes/ECRManaSet.h"
#include "Gameplay/GAS/Attributes/ECRMovementSet.h"
#include "Gameplay/GAS/Components/ECRCharacterHealthComponent.h"
#include "Gameplay/Interaction/InteractionQuery.h"

static FName NAME_ECRCharacterCollisionProfile_Capsule(TEXT("ECRPawnCapsule"));
static FName NAME_ECRCharacterCollisionProfile_Mesh(TEXT("ECRPawnMesh"));

const FName AECRCharacter::NAME_ECRAbilityReady("ECRAbilitiesReady");


AECRCharacter::AECRCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UECRCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	// Avoid ticking characters if possible.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// AbilitySystemComponent needs to be updated at a high frequency.
	NetUpdateFrequency = 100.0f;
	NetCullDistanceSquared = 900000000.0f;

	// Ability system component
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UECRAbilitySystemComponent>(
		this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	AbilitySystemComponent->ReplicationProxyEnabled = true;

	CreateDefaultSubobject<UECRCharacterHealthSet>(TEXT("CharacterHealthSet"));
	CreateDefaultSubobject<UECRCombatSet>(TEXT("CombatSet"));
	CreateDefaultSubobject<UECRMovementSet>(TEXT("MovementSet"));
	CreateDefaultSubobject<UECRManaSet>(TEXT("ManaSet"));

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetCollisionProfileName(NAME_ECRCharacterCollisionProfile_Capsule);

	USkeletalMeshComponent* MeshComp = GetMesh();
	check(MeshComp);
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// Rotate mesh to be X forward since it is exported as Y forward.
	MeshComp->SetCollisionProfileName(NAME_ECRCharacterCollisionProfile_Mesh);

	UECRCharacterMovementComponent* ECRMoveComp = CastChecked<UECRCharacterMovementComponent>(GetCharacterMovement());
	ECRMoveComp->GravityScale = 1.0f;
	ECRMoveComp->MaxAcceleration = 2400.0f;
	ECRMoveComp->BrakingFrictionFactor = 1.0f;
	ECRMoveComp->BrakingFriction = 6.0f;
	ECRMoveComp->GroundFriction = 8.0f;
	ECRMoveComp->BrakingDecelerationWalking = 1400.0f;
	ECRMoveComp->bUseControllerDesiredRotation = false;
	ECRMoveComp->bOrientRotationToMovement = false;
	ECRMoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	ECRMoveComp->bAllowPhysicsRotationDuringAnimRootMotion = false;
	ECRMoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	ECRMoveComp->bCanWalkOffLedgesWhenCrouching = true;
	ECRMoveComp->SetCrouchedHalfHeight(65.0f);

	PawnExtComponent = CreateDefaultSubobject<UECRPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	HealthComponent = CreateDefaultSubobject<UECRCharacterHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	CameraComponent = CreateDefaultSubobject<UECRCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	BaseEyeHeight = 80.0f;
	CrouchedEyeHeight = 50.0f;

	GoingBackwardMultiplier = 1.0f;
	GoingSidewaysMultiplier = 1.0f;
	OrientationToMovementOrientedRequirementAlpha = 0.0f;
}

void AECRCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AECRCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	ActiveGameplayCues.bMinimalReplication = false;
	ActiveGameplayCues.SetOwner(AbilitySystemComponent);	
}

void AECRCharacter::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	const bool bRegisterWithSignificanceManager = !IsNetMode(NM_DedicatedServer);
	if (bRegisterWithSignificanceManager)
	{
		if (UECRSignificanceManager* SignificanceManager = USignificanceManager::Get<UECRSignificanceManager>(World))
		{
			//@TODO: SignificanceManager->RegisterObject(this, (EFortSignificanceType)SignificanceType);
		}
	}

	StartedFallingZ = GetActorLocation().Z;
}

void AECRCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UWorld* World = GetWorld();

	const bool bRegisterWithSignificanceManager = !IsNetMode(NM_DedicatedServer);
	if (bRegisterWithSignificanceManager)
	{
		if (UECRSignificanceManager* SignificanceManager = USignificanceManager::Get<UECRSignificanceManager>(World))
		{
			SignificanceManager->UnregisterObject(this);
		}
	}
}

void AECRCharacter::Reset()
{
	DisableMovementAndCollision();

	K2_OnReset();

	UninitAndDestroy();
}

void AECRCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MainAnimLayer, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RepAnimMontageInfo, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ActiveGameplayCues, SharedParams);

	// Replicate MinimalAscState only to sim proxies
	FDoRepLifetimeParams AscReplicationParams;
	AscReplicationParams.bIsPushBased = true;
	AscReplicationParams.Condition = COND_SimulatedOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MinimalAscState, AscReplicationParams);

}

bool AECRCharacter::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	check(Channel);
	check(Bunch);
	check(RepFlags);

	bool WroteSomething = false;
	for (UActorComponent* ActorComp : ReplicatedComponents)
	{
		if (ActorComp && ActorComp->GetIsReplicated())
		{
			// Replicate everything except ASC for simulated proxies 
			if (ActorComp != AbilitySystemComponent || RepFlags->bNetOwner || !AbilitySystemComponent->
				ReplicationProxyEnabled)
			{
				UActorChannel::SetCurrentSubObjectOwner(ActorComp);
				WroteSomething |= ActorComp->ReplicateSubobjects(Channel, Bunch, RepFlags);
				// Lets the component add subobjects before replicating its own properties.
				UActorChannel::SetCurrentSubObjectOwner(this);
				WroteSomething |= Channel->ReplicateSubobject(ActorComp, *Bunch, *RepFlags);
				// (this makes those subobjects 'supported', and from here on those objects may have reference replicated)	
			}
			else
			{
				// For sim proxies, use MinimalAscState to replicate attributes and tags
				MinimalAscState.FillForCharacter(this);
				if (!MinimalAscState.Equals(LastSharedAscState))
				{
					LastSharedAscState = MinimalAscState;
					MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MinimalAscState, this);
				}
			}
		}
	}

	return WroteSomething;
}

void AECRCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	// In this method fix for root motion desync is implemented from https://github.com/EpicGames/UnrealEngine/commit/fe450c1057b2da0ff5a6d65936a4e3778ce1aa40
	APawn::PreReplication(ChangedPropertyTracker);

	if (GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources() || IsPlayingNetworkedRootMotionMontage())
	{
		const FAnimMontageInstance* RootMotionMontageInstance = GetRootMotionAnimMontageInstance();

		RepRootMotion.bIsActive = true;
		// Is position stored in local space?
		RepRootMotion.bRelativePosition = BasedMovement.HasRelativeLocation();
		RepRootMotion.bRelativeRotation = BasedMovement.HasRelativeRotation();
		RepRootMotion.Location = RepRootMotion.bRelativePosition
			                         ? BasedMovement.Location
			                         : FRepMovement::RebaseOntoZeroOrigin(
				                         GetActorLocation(), GetWorld()->OriginLocation);
		RepRootMotion.Rotation = RepRootMotion.bRelativeRotation ? BasedMovement.Rotation : GetActorRotation();
		RepRootMotion.MovementBase = BasedMovement.MovementBase;
		RepRootMotion.MovementBaseBoneName = BasedMovement.BoneName;
		if (RootMotionMontageInstance)
		{
			RepRootMotion.AnimMontage = RootMotionMontageInstance->Montage;
			RepRootMotion.Position = RootMotionMontageInstance->GetPosition();
		}
		else
		{
			RepRootMotion.AnimMontage = nullptr;
		}

		RepRootMotion.AuthoritativeRootMotion = GetCharacterMovement()->CurrentRootMotion;
		RepRootMotion.Acceleration = GetCharacterMovement()->GetCurrentAcceleration();
		RepRootMotion.LinearVelocity = GetCharacterMovement()->Velocity;

		DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(ACharacter, RepRootMotion, true);
	}
	else
	{
		bWasRootMotionPreviouslyActive = RepRootMotion.bIsActive;
		RepRootMotion.Clear();

		// Replicate RepRootMotion one last time when root motion ends, so that clients see the change.
		// Then deactivate subsequent property comparisons and replication updates until root motion starts again.
		DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(ACharacter, RepRootMotion, bWasRootMotionPreviouslyActive);
	}

	bProxyIsJumpForceApplied = (JumpForceTimeRemaining > 0.0f);
	ReplicatedMovementMode = GetCharacterMovement()->PackNetworkMovementMode();
	ReplicatedBasedMovement = BasedMovement;

	// Optimization: only update and replicate these values if they are actually going to be used.
	if (BasedMovement.HasRelativeLocation())
	{
		// When velocity becomes zero, force replication so the position is updated to match the server (it may have moved due to simulation on the client).
		ReplicatedBasedMovement.bServerHasVelocity = !GetCharacterMovement()->Velocity.IsZero();

		// Make sure absolute rotations are updated in case rotation occurred after the base info was saved.
		if (!BasedMovement.HasRelativeRotation())
		{
			ReplicatedBasedMovement.Rotation = GetActorRotation();
		}
	}

	// Save bandwidth by not replicating this value unless it is necessary, since it changes every update.
	if ((GetCharacterMovement()->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || GetCharacterMovement()->
		bNetworkAlwaysReplicateTransformUpdateTimestamp)
	{
		ReplicatedServerLastTransformUpdateTimeStamp = GetCharacterMovement()->GetServerLastTransformUpdateTimeStamp();
	}
	else
	{
		ReplicatedServerLastTransformUpdateTimeStamp = 0.f;
	}
}

void AECRCharacter::GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const
{
	// Ignore condition for RepRootMotion replication from ACharacter
	APawn::GetReplicatedCustomConditionState(OutActiveState);

	DOREPCUSTOMCONDITION_ACTIVE_FAST(ACharacter, RepRootMotion,
	                                 GetCharacterMovement()->CurrentRootMotion.HasActiveRootMotionSources() ||
	                                 IsPlayingNetworkedRootMotionMontage() || bWasRootMotionPreviouslyActive);
}

void AECRCharacter::ForceReplication()
{
	ForceNetUpdate();
}

void AECRCharacter::Call_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpecForRPC Spec,
	FPredictionKey PredictionKey)
{
	// Don't allow cue executions from periodic effects each periodic tick, as it's network spam and leads to looping cues removal
	if (Spec.Def && Spec.Def->DurationPolicy == EGameplayEffectDurationType::HasDuration && Spec.Def->Period.GetValue() > 0)
	{
		return;
	}

	IECRAbilitySystemReplicationProxyInterface::Call_InvokeGameplayCueExecuted_FromSpec(Spec, PredictionKey);
}

void AECRCharacter::Call_InvokeGameplayCueAdded(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey,
	FGameplayEffectContextHandle EffectContext)
{
}

void AECRCharacter::Call_InvokeGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag,
	FPredictionKey PredictionKey, FGameplayCueParameters Parameters)
{
}

void AECRCharacter::Call_InvokeGameplayCueAddedAndWhileActive_FromSpec(const FGameplayEffectSpecForRPC& Spec,
	FPredictionKey PredictionKey)
{
}

void AECRCharacter::Call_InvokeGameplayCueAddedAndWhileActive_WithParams(const FGameplayTag GameplayCueTag,
	FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
}

void AECRCharacter::Call_InvokeGameplayCuesAddedAndWhileActive_WithParams(const FGameplayTagContainer GameplayCueTags,
	FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
}

void AECRCharacter::NetMulticast_InvokeGameplayCueExecuted_FromSpec_Implementation(
	const FGameplayEffectSpecForRPC Spec, FPredictionKey PredictionKey)
{
	if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	{
		GetAbilitySystemComponent()->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::Executed);
	}
}

void AECRCharacter::NetMulticast_InvokeGameplayCueExecuted_Implementation(
	const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext)
{
	if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	{
		GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed, EffectContext);
	}
}

void AECRCharacter::NetMulticast_InvokeGameplayCuesExecuted_Implementation(
	const FGameplayTagContainer GameplayCueTags, FPredictionKey PredictionKey,
	FGameplayEffectContextHandle EffectContext)
{
	if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	{
		for (const FGameplayTag& GameplayCueTag : GameplayCueTags)
		{
			GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed,
			                                                    EffectContext);
		}
	}
}

void AECRCharacter::NetMulticast_InvokeGameplayCueExecuted_WithParams_Implementation(
	const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
	if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	{
		GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed,
		                                                    GameplayCueParameters);
	}
}

void AECRCharacter::NetMulticast_InvokeGameplayCuesExecuted_WithParams_Implementation(
	const FGameplayTagContainer GameplayCueTags, FPredictionKey PredictionKey,
	FGameplayCueParameters GameplayCueParameters)
{
	if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	{
		for (const FGameplayTag& GameplayCueTag : GameplayCueTags)
		{
			GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed,
			                                                    GameplayCueParameters);
		}
	}
}

void AECRCharacter::NetMulticast_InvokeGameplayCueAdded_Implementation(const FGameplayTag GameplayCueTag,
                                                                       FPredictionKey PredictionKey,
                                                                       FGameplayEffectContextHandle EffectContext)
{
	// if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	// {
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive, EffectContext);
	// }
}

void AECRCharacter::NetMulticast_InvokeGameplayCueAdded_WithParams_Implementation(
	const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters Parameters)
{
	// If server generated prediction key and auto proxy, skip this message. 
	// This is an RPC from mixed replication mode code, we will get the "real" message from our OnRep on the autonomous proxy
	// See UAbilitySystemComponent::AddGameplayCue_Internal for more info.

	// bool bIsMixedReplicationFromServer = (GetAbilitySystemComponent()->ReplicationMode ==
	// 	EGameplayEffectReplicationMode::Mixed && PredictionKey.IsServerInitiatedKey() && IsLocallyControlled());
	//
	// if (HasAuthority() || (PredictionKey.IsLocalClientKey() == false && !bIsMixedReplicationFromServer))
	// {
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive, Parameters);
	// }
}


void AECRCharacter::NetMulticast_InvokeGameplayCueAddedAndWhileActive_FromSpec_Implementation(
	const FGameplayEffectSpecForRPC& Spec, FPredictionKey PredictionKey)
{
	// if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	// {
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::OnActive);
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::WhileActive);
	// }
}

void AECRCharacter::NetMulticast_InvokeGameplayCueAddedAndWhileActive_WithParams_Implementation(
	const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
	// if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	// {
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive,
	// 	                                                    GameplayCueParameters);
	// 	GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::WhileActive,
	// 	                                                    GameplayCueParameters);
	// }
}

void AECRCharacter::NetMulticast_InvokeGameplayCuesAddedAndWhileActive_WithParams_Implementation(
	const FGameplayTagContainer GameplayCueTags, FPredictionKey PredictionKey,
	FGameplayCueParameters GameplayCueParameters)
{
	// if (HasAuthority() || PredictionKey.IsLocalClientKey() == false)
	// {
	// 	for (const FGameplayTag& GameplayCueTag : GameplayCueTags)
	// 	{
	// 		GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive,
	// 		                                                    GameplayCueParameters);
	// 		GetAbilitySystemComponent()->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::WhileActive,
	// 		                                                    GameplayCueParameters);
	// 	}
	// }
}

void AECRCharacter::Call_ReliableGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag,
	FPredictionKey PredictionKey, FGameplayCueParameters Parameters)
{
	ForceReplication();
	ActiveGameplayCues.AddCue(GameplayCueTag, PredictionKey, Parameters);
}

void AECRCharacter::Call_ReliableGameplayCueRemoved(const FGameplayTag GameplayCueTag)
{
	ForceReplication();
	ActiveGameplayCues.RemoveCue(GameplayCueTag);
}

FGameplayAbilityRepAnimMontage& AECRCharacter::Call_GetRepAnimMontageInfo_Mutable()
{
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RepAnimMontageInfo, this);
	return RepAnimMontageInfo;
}

void AECRCharacter::Call_OnRep_ReplicatedAnimMontage()
{
	UECRAbilitySystemComponent* ASC = GetECRAbilitySystemComponent();
	if (ASC)
	{
		// Update ASC client version of RepAnimMontageInfo
		ASC->SetRepAnimMontageInfoAccessor(RepAnimMontageInfo);
		// Call OnRep of AnimMontageInfo
		ASC->ReplicatedAnimMontageOnRepAccesor();
	}
}


void AECRCharacter::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                             FInteractionOptionBuilder& OptionBuilder)
{
	TArray<FInteractionOption> InteractionOptions = GetInteractionOptions(InteractQuery);
	for (FInteractionOption InteractionOption : InteractionOptions)
	{
		OptionBuilder.AddInteractionOption(InteractionOption);
	}
}

AECRPlayerController* AECRCharacter::GetECRPlayerController() const
{
	return CastChecked<AECRPlayerController>(Controller, ECastCheckedType::NullAllowed);
}

AECRPlayerState* AECRCharacter::GetECRPlayerState() const
{
	return CastChecked<AECRPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UECRAbilitySystemComponent* AECRCharacter::GetECRAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAbilitySystemComponent* AECRCharacter::GetAbilitySystemComponent() const
{
	return GetECRAbilitySystemComponent();
}

void AECRCharacter::OnAbilitySystemInitialized()
{
	UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent();
	check(ECRASC);

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		InitPawnDataAndAbilities();
	}

	HealthComponent->InitializeWithAbilitySystem(ECRASC);

	InitializeGameplayTags();
}

void AECRCharacter::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AECRCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		PawnExtComponent->SetPawnData(PawnData);
	}

	SetOwner(NewController);
}

void AECRCharacter::UnPossessed()
{
	AController* const OldController = Controller;

	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();
}

void AECRCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void AECRCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
	K2_OnPlayerStateChanged();
}

void AECRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AECRCharacter::InitializeGameplayTags()
{
	// Clear tags that may be lingering on the ability system from the previous pawn.
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();

		for (const TPair<uint8, FGameplayTag>& TagMapping : GameplayTags.MovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				ECRASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		for (const TPair<uint8, FGameplayTag>& TagMapping : GameplayTags.CustomMovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				ECRASC->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		// Two blocking movement modes
		ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_Standard, 0);
		ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_JumpPack, 0);

		UECRCharacterMovementComponent* ECRMoveComp = CastChecked<UECRCharacterMovementComponent>(
			GetCharacterMovement());
		SetMovementModeTag(ECRMoveComp->MovementMode, ECRMoveComp->CustomMovementMode, true);
	}
}

void AECRCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool AECRCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool AECRCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool AECRCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void AECRCharacter::SetMainAnimLayer(TSubclassOf<UAnimInstance> InLayer)
{
	MainAnimLayer = InLayer;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MainAnimLayer, this);
}

void AECRCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	HealthComponent->DamageSelfDestruct(/*bFellOutOfWorld=*/ true);
}

void AECRCharacter::OnDeathStarted(AActor*)
{
	DisableMovementAndCollision();
}

void AECRCharacter::OnDeathFinished(AActor*)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}


void AECRCharacter::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UECRCharacterMovementComponent* ECRMoveComp = CastChecked<UECRCharacterMovementComponent>(GetCharacterMovement());
	ECRMoveComp->StopMovementImmediately();
	ECRMoveComp->DisableMovement();
}

void AECRCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}


void AECRCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();
		SetLifeSpan(0.1f);
	}

	// Uninitialize the ASC if we're still the avatar actor (otherwise another pawn already did it when they became the avatar actor)
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		if (ECRASC->GetAvatarActor() == this)
		{
			PawnExtComponent->UninitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}

void AECRCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UECRCharacterMovementComponent* ECRMoveComp = CastChecked<UECRCharacterMovementComponent>(
		GetCharacterMovement());

	// Sending movement tag for possible blocking of some abilities
	SetMovementModeTag(PrevMovementMode, PreviousCustomMode, false);
	SetMovementModeTag(ECRMoveComp->MovementMode, ECRMoveComp->CustomMovementMode, true);

	if (ECRMoveComp->MovementMode == MOVE_Falling)
	{
		StartedFallingTime = GetWorld()->GetTimeSeconds();
		StartedFallingZ = GetActorLocation().Z;
	}
	else if (ECRMoveComp->MovementMode == MOVE_Walking)
	{
		if (PrevMovementMode == MOVE_Falling)
		{
			if (GetECRAbilitySystemComponent())
			{
				const float TimeFalling = GetWorld()->GetTimeSeconds() - StartedFallingTime;
				const float DistanceFalling = StartedFallingZ - GetActorLocation().Z;

				// Sending gameplay event for landing
				FGameplayEventData Payload;
				Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_Landed;
				Payload.Target = this;
				Payload.EventMagnitude = DistanceFalling;

				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
					this, FECRGameplayTags::Get().GameplayEvent_Landed, Payload);
			}
		}
	}

	if (GetECRAbilitySystemComponent())
	{
		// Sending gameplay event for possible interruption of some abilities
		FGameplayEventData Payload;
		Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_MovementModeChanged;
		Payload.Target = this;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this, FECRGameplayTags::Get().GameplayEvent_MovementModeChanged, Payload);
	}
}

void AECRCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();
		const FGameplayTag* MovementModeTag = nullptr;

		if (MovementMode == MOVE_Custom)
		{
			MovementModeTag = GameplayTags.CustomMovementModeTagMap.Find(CustomMovementMode);
		}
		else
		{
			MovementModeTag = GameplayTags.MovementModeTagMap.Find(MovementMode);
		}

		if (MovementModeTag && MovementModeTag->IsValid())
		{
			ECRASC->SetLooseGameplayTagCount(*MovementModeTag, (bTagEnabled ? 1 : 0));

			// Set falling tag depending on whether jump pack or not
			if (*MovementModeTag == GameplayTags.Movement_Mode_Falling)
			{
				if (!bTagEnabled)
				{
					ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_Standard, 0);
					ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_JumpPack, 0);
				}
				else
				{
					if (ECRASC->HasMatchingGameplayTag(GameplayTags.Status_JumpFlying))
					{
						ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_JumpPack, 1);
					}
					else
					{
						ECRASC->SetLooseGameplayTagCount(GameplayTags.Movement_Mode_Falling_Standard, 1);
					}
				}
			}
		}
	}
}

void AECRCharacter::ToggleCrouch()
{
	const UECRCharacterMovementComponent* ECRMoveComp = CastChecked<UECRCharacterMovementComponent>(
		GetCharacterMovement());

	if (bIsCrouched || ECRMoveComp->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (ECRMoveComp->IsMovingOnGround())
	{
		Crouch();
	}
}

void AECRCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_Crouching, 1);
	}


	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AECRCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool AECRCharacter::CanJumpInternal_Implementation() const
{
	// same as ACharacter's implementation but without the crouch check
	return JumpIsAllowedInternal();
}


void AECRCharacter::GrantAbilitySets(TArray<UECRAbilitySet*> AbilitySets) const
{
	for (const UECRAbilitySet* AbilitySet : AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}
}

void AECRCharacter::InitPawnDataAndAbilities()
{
	ensureMsgf(PawnData, TEXT("ECRCharacter [%s] pawn data is not specified"), *(GetNameSafe(this)));

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);

	// Granting common ability sets from game state
	if (AECRGameState* GameState = Cast<AECRGameState>(GetWorld()->GetGameState()))
	{
		const TArray<UECRAbilitySet*> CommonCharacterAbilitySets = GameState->GetCommonCharacterAbilitySets();
		GrantAbilitySets(CommonCharacterAbilitySets);
	}

	// Granting this character ability sets
	GrantAbilitySets(PawnData->AbilitySets);

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_ECRAbilityReady);

	ForceNetUpdate();
}

void AECRCharacter::OnRep_PawnData()
{
}

void AECRCharacter::OnRep_MainAnimLayer()
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		if (USkeletalMeshComponent* MyMesh = GetMesh())
		{
			MyMesh->LinkAnimClassLayers(MainAnimLayer);
		}
	}
}

void AECRCharacter::OnRep_MinimalAscState()
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		UECRAbilitySystemComponent* ASC = GetECRAbilitySystemComponent();
		if (ASC)
		{
			// Update ASC client attributes
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetShieldAttribute(), MinimalAscState.Shield);
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetMaxShieldAttribute(), 100.0f);
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetHealthAttribute(), MinimalAscState.Health);
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetMaxHealthAttribute(), 100.0f);
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetBleedingHealthAttribute(),
			                             MinimalAscState.BleedingHealth);
			ASC->SetNumericAttributeBase(UECRCharacterHealthSet::GetMaxBleedingHealthAttribute(), 100.0f);
			ASC->SetNumericAttributeBase(UECRCombatSet::GetArmorAttribute(), 100.0f);

			// Update gameplay tags
			const TArray<FGameplayTag>& RepTags = FECRGameplayTags::Get().SimProxyReplicatedTags;
			for (int32 i = 0; i < RepTags.Num(); i++)
			{
				if ((MinimalAscState.GameplayTagsBitMask & (1 << i)) != 0)
				{
					ASC->SetLooseGameplayTagCount(RepTags[i], 1);
				}
				else
				{
					ASC->SetLooseGameplayTagCount(RepTags[i], 0);
				}
			}
		}
	}
}


bool AECRCharacter::UpdateSharedReplication()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (IsPlayingNetworkedRootMotionMontage())
		{
			return false;
		}

		// Movement
		FSharedRepMovement SharedMovement;
		if (!SharedMovement.FillForCharacter(this))
		{
			return false;
		}

		// Only call FastSharedReplication if data has changed since the last frame.
		// Skipping this call will cause replication to reuse the same bunch that we previously
		// produced, but not send it to clients that already received. (But a new client who has not received
		// it, will get it this frame)
		float MinTimeToUpdate = (MinNetUpdateFrequency != 0) ? (1 / MinNetUpdateFrequency) : 0.5f;
		if ((GetGameTimeSinceCreation() - LastSharedReplicationTimestamp >= MinTimeToUpdate) || !SharedMovement.Equals(
			LastSharedReplication, this))
		{
			LastSharedReplication = SharedMovement;
			ReplicatedMovementMode = SharedMovement.RepMovementMode;
			LastSharedReplicationTimestamp = GetGameTimeSinceCreation();

			FastSharedMovementReplication(SharedMovement);
		}

		return true;
	}

	// We cannot fastrep right now. Don't send anything.
	return false;
}

void AECRCharacter::FastSharedMovementReplication_Implementation(const FSharedRepMovement& SharedRepMovement)
{
	if (GetWorld()->IsPlayingReplay())
	{
		return;
	}

	// Timestamp is checked to reject old moves.
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		// Timestamp
		ReplicatedServerLastTransformUpdateTimeStamp = SharedRepMovement.RepTimeStamp;

		// Movement mode
		if (ReplicatedMovementMode != SharedRepMovement.RepMovementMode)
		{
			ReplicatedMovementMode = SharedRepMovement.RepMovementMode;
			GetCharacterMovement()->bNetworkMovementModeChanged = true;
			GetCharacterMovement()->bNetworkUpdateReceived = true;
		}

		// Location, Rotation, Velocity, etc.
		FRepMovement& MutableRepMovement = GetReplicatedMovement_Mutable();
		MutableRepMovement = SharedRepMovement.RepMovement;

		// This also sets LastRepMovement
		OnRep_ReplicatedMovement();

		// Jump force
		bProxyIsJumpForceApplied = SharedRepMovement.bProxyIsJumpForceApplied;

		// Crouch
		if (bIsCrouched != SharedRepMovement.bIsCrouched)
		{
			bIsCrouched = SharedRepMovement.bIsCrouched;
			OnRep_IsCrouched();
		}
	}
}

FSharedRepMovement::FSharedRepMovement()
{
	RepMovement.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;
}

bool FSharedRepMovement::FillForCharacter(ACharacter* Character)
{
	if (USceneComponent* PawnRootComponent = Character->GetRootComponent())
	{
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();

		RepMovement.Location = FRepMovement::RebaseOntoZeroOrigin(PawnRootComponent->GetComponentLocation(), Character);
		RepMovement.Rotation = PawnRootComponent->GetComponentRotation();
		RepMovement.LinearVelocity = CharacterMovement->Velocity;
		RepMovementMode = CharacterMovement->PackNetworkMovementMode();
		bProxyIsJumpForceApplied = Character->bProxyIsJumpForceApplied || (Character->JumpForceTimeRemaining > 0.0f);
		bIsCrouched = Character->bIsCrouched;

		// Timestamp is sent as zero if unused
		if ((CharacterMovement->NetworkSmoothingMode == ENetworkSmoothingMode::Linear) || CharacterMovement->
			bNetworkAlwaysReplicateTransformUpdateTimestamp)
		{
			RepTimeStamp = CharacterMovement->GetServerLastTransformUpdateTimeStamp();
		}
		else
		{
			RepTimeStamp = 0.f;
		}

		return true;
	}
	return false;
}

bool FSharedRepMovement::Equals(const FSharedRepMovement& Other, ACharacter* Character) const
{
	if (RepMovement.Location != Other.RepMovement.Location)
	{
		return false;
	}

	if (RepMovement.Rotation != Other.RepMovement.Rotation)
	{
		return false;
	}

	if (RepMovement.LinearVelocity != Other.RepMovement.LinearVelocity)
	{
		return false;
	}

	if (RepMovementMode != Other.RepMovementMode)
	{
		return false;
	}

	if (bProxyIsJumpForceApplied != Other.bProxyIsJumpForceApplied)
	{
		return false;
	}

	if (bIsCrouched != Other.bIsCrouched)
	{
		return false;
	}

	return true;
}

bool FSharedRepMovement::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;
	RepMovement.NetSerialize(Ar, Map, bOutSuccess);
	Ar << RepMovementMode;
	Ar << bProxyIsJumpForceApplied;
	Ar << bIsCrouched;

	// Timestamp, if non-zero.
	uint8 bHasTimeStamp = (RepTimeStamp != 0.f);
	Ar.SerializeBits(&bHasTimeStamp, 1);
	if (bHasTimeStamp)
	{
		Ar << RepTimeStamp;
	}
	else
	{
		RepTimeStamp = 0.f;
	}

	return true;
}

FMinimalASCState::FMinimalASCState()
{
}

uint8 ConvertForNetToPercent(float Value, float MaxValue)
{
	const float Percent = MaxValue != 0 ? (Value / MaxValue * 100) : 0;
	return FMath::Clamp(FMath::RoundToInt(Percent), 0, 100);
}

bool FMinimalASCState::FillForCharacter(AECRCharacter* Character)
{
	if (UECRAbilitySystemComponent* ASC = Character->GetECRAbilitySystemComponent())
	{
		// Health, shield and bleeding health for sim proxies only need percents (0-100) to display progress bars, so use uint8
		Health = ConvertForNetToPercent(ASC->GetNumericAttribute(UECRCharacterHealthSet::GetHealthAttribute()),
		                                ASC->GetNumericAttribute(UECRCharacterHealthSet::GetMaxHealthAttribute()));
		Shield = ConvertForNetToPercent(ASC->GetNumericAttribute(UECRCharacterHealthSet::GetShieldAttribute()),
		                                ASC->GetNumericAttribute(UECRCharacterHealthSet::GetMaxShieldAttribute()));
		BleedingHealth = ConvertForNetToPercent(
			ASC->GetNumericAttribute(UECRCharacterHealthSet::GetBleedingHealthAttribute()),
			ASC->GetNumericAttribute(UECRCharacterHealthSet::GetMaxBleedingHealthAttribute()));
		// Armor needs full float value, but will be optimized in NetSerialize since in 99.9% cases it's 100
		Armor = ASC->GetNumericAttribute(UECRCombatSet::GetArmorAttribute());

		// Filling mask of gameplay tags for sim proxies
		uint16 NewMask = 0;
		const TArray<FGameplayTag>& RepTags = FECRGameplayTags::Get().SimProxyReplicatedTags;
		for (int32 i = 0; i < FMath::Min(RepTags.Num(), 16); i++)
		{
			if (ASC->HasMatchingGameplayTag(RepTags[i]))
			{
				NewMask |= (1 << i);
			}
		}
		GameplayTagsBitMask = NewMask;

		return true;
	}

	return false;
}

bool FMinimalASCState::Equals(const FMinimalASCState& Other) const
{
	if (GameplayTagsBitMask != Other.GameplayTagsBitMask)
	{
		return false;
	}

	if (Shield != Other.Shield)
	{
		return false;
	}

	if (Health != Other.Health)
	{
		return false;
	}

	if (BleedingHealth != Other.BleedingHealth)
	{
		return false;
	}

	return true;
}

bool FMinimalASCState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = true;

	// Always send tag mask (cheap, important for gameplay)
	Ar << GameplayTagsBitMask;

	// -------------------------------------------------
	// Pack presence flags into 1 nibble (4 bits)
	// -------------------------------------------------
	uint8 PresenceFlags = 0;

	// bit 0: Health != 100%
	// bit 1: Shield != 100%
	// bit 2: BleedingHealth != 100%
	// bit 3: Armor != 100

	if (Ar.IsSaving())
	{
		PresenceFlags |= (Health != 100) << 0;
		PresenceFlags |= (Shield != 100) << 1;
		PresenceFlags |= (BleedingHealth != 100) << 2;
		PresenceFlags |= (!FMath::IsNearlyEqual(Armor, 100.0f, 0.1f)) << 3;
	}

	Ar.SerializeBits(&PresenceFlags, 4);

	// -------------------------------------------------
	// HEALTH
	// -------------------------------------------------
	if (PresenceFlags & (1 << 0))
	{
		Ar << Health;
	}
	else if (Ar.IsLoading())
	{
		Health = 100;
	}

	// -------------------------------------------------
	// SHIELD
	// -------------------------------------------------
	if (PresenceFlags & (1 << 1))
	{
		Ar << Shield;
	}
	else if (Ar.IsLoading())
	{
		Shield = 100;
	}

	// -------------------------------------------------
	// BLEEDING
	// -------------------------------------------------
	if (PresenceFlags & (1 << 2))
	{
		Ar << BleedingHealth;
	}
	else if (Ar.IsLoading())
	{
		BleedingHealth = 100;
	}

	// -------------------------------------------------
	// ARMOR
	// -------------------------------------------------
	if (PresenceFlags & (1 << 3))
	{
		Ar << Armor;
	}
	else if (Ar.IsLoading())
	{
		Armor = 100.f;
	}

	return true;
}
