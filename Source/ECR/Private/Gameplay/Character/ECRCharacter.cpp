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
#include "Gameplay/ECRGameState.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
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

	NetCullDistanceSquared = 900000000.0f;

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
}

void AECRCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
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

	DOREPLIFETIME_CONDITION(ThisClass, ReplicatedAcceleration, COND_SimulatedOnly);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
}

void AECRCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		// Compress Acceleration: XY components as direction + magnitude, Z component as direct value
		const double MaxAccel = MovementComponent->MaxAcceleration;
		const FVector CurrentAccel = MovementComponent->GetCurrentAcceleration();
		double AccelXYRadians, AccelXYMagnitude;
		FMath::CartesianToPolar(CurrentAccel.X, CurrentAccel.Y, AccelXYMagnitude, AccelXYRadians);

		ReplicatedAcceleration.AccelXYRadians = FMath::FloorToInt((AccelXYRadians / TWO_PI) * 255.0);
		// [0, 2PI] -> [0, 255]
		ReplicatedAcceleration.AccelXYMagnitude = FMath::FloorToInt((AccelXYMagnitude / MaxAccel) * 255.0);
		// [0, MaxAccel] -> [0, 255]
		ReplicatedAcceleration.AccelZ = FMath::FloorToInt((CurrentAccel.Z / MaxAccel) * 127.0);
		// [-MaxAccel, MaxAccel] -> [-127, 127]
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
	return Cast<UECRAbilitySystemComponent>(GetAbilitySystemComponent());
}

UAbilitySystemComponent* AECRCharacter::GetAbilitySystemComponent() const
{
	if (PawnExtComponent)
	{
		return PawnExtComponent->GetECRAbilitySystemComponent();
	}
	return nullptr;
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
	}
	else if (PrevMovementMode == MOVE_Falling)
	{
		float TimeFalling = GetWorld()->GetTimeSeconds() - StartedFallingTime;

		// Sending gameplay event for possible fall damage
		if (GetECRAbilitySystemComponent() != nullptr)
		{
			FGameplayEventData Payload;
			Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_Landed;
			Payload.Target = this;
			Payload.EventMagnitude = TimeFalling;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				this, FECRGameplayTags::Get().GameplayEvent_Landed, Payload);
		}
	}
	// Sending gameplay event for possible interruption of some abilities
	if (GetECRAbilitySystemComponent() != nullptr)
	{
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
			UECRAbilitySystemComponent* ECRAbilitySystemComponent = GetECRPlayerState()->GetECRAbilitySystemComponent();
			AbilitySet->GiveToAbilitySystem(ECRAbilitySystemComponent, nullptr);
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

void AECRCharacter::OnRep_ReplicatedAcceleration()
{
	if (UECRCharacterMovementComponent* ECRMovementComponent = Cast<UECRCharacterMovementComponent>(
		GetCharacterMovement()))
	{
		// Decompress Acceleration
		const double MaxAccel = ECRMovementComponent->MaxAcceleration;
		const double AccelXYMagnitude = double(ReplicatedAcceleration.AccelXYMagnitude) * MaxAccel / 255.0;
		// [0, 255] -> [0, MaxAccel]
		const double AccelXYRadians = double(ReplicatedAcceleration.AccelXYRadians) * TWO_PI / 255.0;
		// [0, 255] -> [0, 2PI]

		FVector UnpackedAcceleration(FVector::ZeroVector);
		FMath::PolarToCartesian(AccelXYMagnitude, AccelXYRadians, UnpackedAcceleration.X, UnpackedAcceleration.Y);
		UnpackedAcceleration.Z = double(ReplicatedAcceleration.AccelZ) * MaxAccel / 127.0;
		// [-127, 127] -> [-MaxAccel, MaxAccel]

		ECRMovementComponent->SetReplicatedAcceleration(UnpackedAcceleration);
	}
}
