// Copyleft: All rights reversed


#include "Gameplay/Player/ECRPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Framework/Application/NavigationConfig.h"
#include "Gameplay/Camera/ECRPlayerCameraManager.h"
#include "Gameplay/Player/ECRPlayerState.h"


AECRPlayerController::AECRPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = AECRPlayerCameraManager::StaticClass();
	bInvertCameraY = false;
	DesiredCameraDistanceMultiplier = 1.0f;
	CameraDistanceMultiplier = DesiredCameraDistanceMultiplier;
	CameraDistanceInterpolationSpeed = 1.0f;
}


AECRPlayerState* AECRPlayerController::GetECRPlayerState() const
{
	return CastChecked<AECRPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UECRAbilitySystemComponent* AECRPlayerController::GetECRAbilitySystemComponent() const
{
	const AECRPlayerState* ECRPS = GetECRPlayerState();
	if (ECRPS)
	{
		if (APawn* MyPawn = ECRPS->GetPawn())
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(MyPawn))
			{
				if (UECRAbilitySystemComponent* ECRASC = Cast<UECRAbilitySystemComponent>(
					ASI->GetAbilitySystemComponent()))
				{
					return ECRASC;
				}
			}
		}
	}
	return nullptr;
}

void AECRPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AECRPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		const TSharedRef<FNavigationConfig> Navigation = MakeShared<FNavigationConfig>();
		Navigation->bKeyNavigation = false;
		Navigation->bTabNavigation = false;
		Navigation->bAnalogNavigation = false;
		FSlateApplication::Get().SetNavigationConfig(Navigation);
	}
}


void AECRPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AECRPlayerController::OnUnPossess()
{
	// Make sure the pawn that is being unpossessed doesn't remain our ASC's avatar actor
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}

	Super::OnUnPossess();
}

void AECRPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// If we are auto running then add some player input
	if (GetIsAutoRunning())
	{
		if (APawn* CurrentPawn = GetPawn())
		{
			const FRotator MovementRotation(0.0f, GetControlRotation().Yaw, 0.0f);
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			CurrentPawn->AddMovementInput(MovementDirection, 1.0f);
		}
	}

	// Interpolating camera distance multiplier
	CameraDistanceMultiplier = FMath::FInterpTo(CameraDistanceMultiplier, DesiredCameraDistanceMultiplier, DeltaTime,
	                                            CameraDistanceInterpolationSpeed);
}

void AECRPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetIsAutoRunning(false);
}

void AECRPlayerController::SetIsAutoRunning(const bool bEnabled)
{
	const bool bIsAutoRunning = GetIsAutoRunning();
	if (bEnabled != bIsAutoRunning)
	{
		if (!bEnabled)
		{
			OnEndAutoRun();
		}
		else
		{
			OnStartAutoRun();
		}
	}
}

bool AECRPlayerController::GetIsAutoRunning() const
{
	bool bIsAutoRunning = false;
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		bIsAutoRunning = ECRASC->GetTagCount(FECRGameplayTags::Get().Status_AutoRunning) > 0;
	}
	return bIsAutoRunning;
}

int32 AECRPlayerController::GetInPacketLoss() const
{
	if (const UNetConnection* MyNetConnection = GetNetConnection())
	{
		return MyNetConnection->InPacketsLost;
	}
	return 0;
}

int32 AECRPlayerController::GetOutPacketLoss() const
{
	if (const UNetConnection* MyNetConnection = GetNetConnection())
	{
		return MyNetConnection->OutPacketsLost;
	}
	return 0;
}

void AECRPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		UE_LOG(LogTemp, Warning, TEXT("Requesting process from %s"), *GetNameSafe(this))
		ECRASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AECRPlayerController::OnStartAutoRun()
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_AutoRunning, 1);
		K2_OnStartAutoRun();
	}
}

void AECRPlayerController::OnEndAutoRun()
{
	if (UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->SetLooseGameplayTagCount(FECRGameplayTags::Get().Status_AutoRunning, 0);
		K2_OnEndAutoRun();
	}
}

//////////////////////////////////////////////////////////////////////
// AECRReplayPlayerController

void AECRReplayPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
	K2_OnPlayerSet();
}
