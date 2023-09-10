// Copyleft: All rights reversed


#include "Gameplay/Vehicles/ECRWheeledVehiclePawn.h"

#include "ChaosVehicleMovementComponent.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Gameplay/GAS/Attributes/ECRSimpleVehicleHealthSet.h"
#include "Gameplay/GAS/Components/ECRHealthComponent.h"
#include "Gameplay/Player/ECRPlayerState.h"
#include "Net/UnrealNetwork.h"


AECRWheeledVehiclePawn::AECRWheeledVehiclePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Avoid ticking vehicles if possible.
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

	HealthSet = CreateDefaultSubobject<UECRSimpleVehicleHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UECRCombatSet>(TEXT("CombatSet"));

	PawnExtComponent = CreateDefaultSubobject<UECRPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	HealthComponent = CreateDefaultSubobject<UECRHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	CameraComponent = CreateDefaultSubobject<UECRCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
}

AECRPlayerController* AECRWheeledVehiclePawn::GetECRPlayerController() const
{
	return CastChecked<AECRPlayerController>(Controller, ECastCheckedType::NullAllowed);
}

AECRPlayerState* AECRWheeledVehiclePawn::GetECRPlayerState() const
{
	return CastChecked<AECRPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UECRAbilitySystemComponent* AECRWheeledVehiclePawn::GetECRAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAbilitySystemComponent* AECRWheeledVehiclePawn::GetAbilitySystemComponent() const
{
	return GetECRAbilitySystemComponent();
}

void AECRWheeledVehiclePawn::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool AECRWheeledVehiclePawn::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool AECRWheeledVehiclePawn::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool AECRWheeledVehiclePawn::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void AECRWheeledVehiclePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
}

void AECRWheeledVehiclePawn::OnAbilitySystemInitialized()
{
	UE_LOG(LogTemp, Warning, TEXT("vehicle Abilitysystem initialized"))
	UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent();
	check(ECRASC);

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		InitPawnDataAndAbilities();
	}

	HealthComponent->InitializeWithAbilitySystem(ECRASC);
}

void AECRWheeledVehiclePawn::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AECRWheeledVehiclePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		PawnExtComponent->SetPawnData(PawnData);
	}
}

void AECRWheeledVehiclePawn::UnPossessed()
{
	AController* const OldController = Controller;

	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();
}

void AECRWheeledVehiclePawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void AECRWheeledVehiclePawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void AECRWheeledVehiclePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}

void AECRWheeledVehiclePawn::GrantAbilitySets(TArray<UECRAbilitySet*> AbilitySets) const
{
	UE_LOG(LogTemp, Warning, TEXT("Starting granting vehicles ability sets"))
	for (const UECRAbilitySet* AbilitySet : AbilitySets)
	{
		if (AbilitySystemComponent && AbilitySet)
		{
			UE_LOG(LogTemp, Warning, TEXT("Granting vehicles ability set"))
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}
}

void AECRWheeledVehiclePawn::OnDeathStarted(AActor* OwningActor)
{
	DisableMovementAndCollision();
}

void AECRWheeledVehiclePawn::OnDeathFinished(AActor* OwningActor)
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}

void AECRWheeledVehiclePawn::DisableMovementAndCollision()
{
	if (Controller)
	{
		Controller->SetIgnoreMoveInput(true);
	}

	USkeletalMeshComponent* SkeletalMeshComponent = GetMesh();
	check(SkeletalMeshComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

	UChaosVehicleMovementComponent* MovementComponent = GetVehicleMovement();
	MovementComponent->StopMovementImmediately();
}

void AECRWheeledVehiclePawn::DestroyDueToDeath()
{
	K2_OnDeathFinished();

	UninitAndDestroy();
}

void AECRWheeledVehiclePawn::UninitAndDestroy()
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

void AECRWheeledVehiclePawn::InitPawnDataAndAbilities()
{
	ensureMsgf(PawnData, TEXT("ECRCharacter [%s] pawn data is not specified"), *(GetNameSafe(this)));

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);

	// Granting this character ability sets
	GrantAbilitySets(PawnData->AbilitySets);

	ForceNetUpdate();
}

void AECRWheeledVehiclePawn::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	FInteractionOptionBuilder& OptionBuilder)
{
	TArray<FInteractionOption> InteractionOptions = GetInteractionOptions(InteractQuery);
	for (FInteractionOption InteractionOption : InteractionOptions)
	{
		OptionBuilder.AddInteractionOption(InteractionOption);
	}
}

void AECRWheeledVehiclePawn::OnRep_PawnData()
{
}
