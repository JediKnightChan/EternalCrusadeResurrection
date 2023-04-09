// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Character/ECRHeroComponent.h"
#include "System/ECRLogChannels.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Gameplay/Player/ECRPlayerState.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Input/ECRInputConfig.h"
#include "Input/ECRInputComponent.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Settings/ECRSettingsLocal.h"
#include "PlayerMappableInputConfig.h"
#include "Gameplay/Camera/ECRCameraMode.h"

#if WITH_EDITOR
#include "Misc/UObjectToken.h"
#endif	// WITH_EDITOR

namespace ECRHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UECRHeroComponent::NAME_BindInputsNow("BindInputsNow");

UECRHeroComponent::UECRHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilityCameraMode = nullptr;
	bPawnHasInitialized = false;
	bReadyToBindInputs = false;
	bMovementInputEnabled = true;
}

void UECRHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtComp->OnPawnReadyToInitialize_RegisterAndCall(
				FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnPawnReadyToInitialize));
		}
	}
	else
	{
		UE_LOG(LogECR, Error,
		       TEXT(
			       "[UECRHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."
		       ));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("ECRHeroComponent", "NotOnPawnError",
			                                       "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName HeroMessageLogName = TEXT("ECRHeroComponent");

			FMessageLog(HeroMessageLogName).Error()
			                               ->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
			                               ->AddToken(FTextToken::Create(Message));

			FMessageLog(HeroMessageLogName).Open();
		}
#endif
	}
}

bool UECRHeroComponent::IsPawnComponentReadyToInitialize() const
{
	// The player state is required.
	if (!GetPlayerState<AECRPlayerState>())
	{
		return false;
	}

	const APawn* Pawn = GetPawn<APawn>();

	// A pawn is required.
	if (!Pawn)
	{
		return false;
	}

	// If we're authority or autonomous, we need to wait for a controller with registered ownership of the player state.
	if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
	{
		AController* Controller = GetController<AController>();

		const bool bHasControllerPairedWithPS = (Controller != nullptr) &&
			(Controller->PlayerState != nullptr) &&
			(Controller->PlayerState->GetOwner() == Controller);

		if (!bHasControllerPairedWithPS)
		{
			return false;
		}
	}

	const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
	const bool bIsBot = Pawn->IsBotControlled();

	if (bIsLocallyControlled && !bIsBot)
	{
		// The input component is required when locally controlled.
		if (!Pawn->InputComponent)
		{
			return false;
		}
	}

	return true;
}

void UECRHeroComponent::OnPawnReadyToInitialize()
{
	if (!ensure(!bPawnHasInitialized))
	{
		// Don't initialize twice
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}
	const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

	AECRPlayerState* ECRPS = GetPlayerState<AECRPlayerState>();
	check(ECRPS);

	const UECRPawnData* PawnData = nullptr;

	if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		PawnData = PawnExtComp->GetPawnData<UECRPawnData>();

		// The player state holds the persistent data for this player (state that persists across deaths and multiple pawns).
		// The ability system component and attribute sets live on the player state.
		PawnExtComp->InitializeAbilitySystem(ECRPS->GetECRAbilitySystemComponent(), ECRPS);
	}

	if (AECRPlayerController* ECRPC = GetController<AECRPlayerController>())
	{
		if (Pawn->InputComponent != nullptr)
		{
			InitializePlayerInput(Pawn->InputComponent);
		}
	}

	if (bIsLocallyControlled && PawnData)
	{
		if (UECRCameraComponent* CameraComponent = UECRCameraComponent::FindCameraComponent(Pawn))
		{
			CameraComponent->DetermineCameraModeDelegate.BindUObject(this, &ThisClass::DetermineCameraMode);
		}
	}

	bPawnHasInitialized = true;
}

void UECRHeroComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UECRHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtComp->UninitializeAbilitySystem();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UECRHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	Subsystem->ClearAllMappings();

	if (const UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UECRPawnData* PawnData = PawnExtComp->GetPawnData<UECRPawnData>())
		{
			if (const UECRInputConfig* InputConfig = PawnData->InputConfig)
			{
				const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();

				// Register any default input configs with the settings so that they will be applied to the player during AddInputMappings
				for (const FMappableConfigPair& Pair : DefaultInputConfigs)
				{
					if (Pair.bShouldActivateAutomatically && Pair.CanBeActivated())
					{
						FModifyContextOptions Options = {};
						Options.bIgnoreAllPressedKeysUntilRelease = false;
						// Actually add the config to the local player							
						Subsystem->AddPlayerMappableConfig(Pair.Config.LoadSynchronous(), Options);	
					}
				}

				UECRInputComponent* ECRIC = CastChecked<UECRInputComponent>(PlayerInputComponent);
				ECRIC->AddInputMappings(InputConfig, Subsystem);

				TArray<uint32> BindHandles;
				ECRIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
				                          &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

				ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Move, ETriggerEvent::Triggered, this,
				                        &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
				ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Look_Mouse, ETriggerEvent::Triggered, this,
				                        &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ true);
				ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Look_Stick, ETriggerEvent::Triggered, this,
				                        &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
				ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Crouch, ETriggerEvent::Triggered, this,
				                        &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
				ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_AutoRun, ETriggerEvent::Triggered, this,
				                        &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
			}
		}
	}

	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		const_cast<APlayerController*>(PC), NAME_BindInputsNow);
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
		const_cast<APawn*>(Pawn), NAME_BindInputsNow);
}


void UECRHeroComponent::AddAdditionalInputConfig(const UECRInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	UECRInputComponent* ECRIC = Pawn->FindComponentByClass<UECRInputComponent>();
	check(ECRIC);

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	if (const UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		ECRIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
		                          &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
	}
}

void UECRHeroComponent::RemoveAdditionalInputConfig(const UECRInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UECRHeroComponent::HasPawnInitialized() const
{
	return bPawnHasInitialized;
}

bool UECRHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UECRHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const UECRPawnExtensionComponent* PawnExtComp =
			UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UECRAbilitySystemComponent* ECRASC = PawnExtComp->GetECRAbilitySystemComponent())
			{
				ECRASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UECRHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UECRAbilitySystemComponent* ECRASC = PawnExtComp->GetECRAbilitySystemComponent())
		{
			ECRASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UECRHeroComponent::ToggleMovementInput(const bool bNewEnabled)
{
	bMovementInputEnabled = bNewEnabled;
}

void UECRHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	if (!bMovementInputEnabled)
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	if (AECRPlayerController* ECRController = Cast<AECRPlayerController>(Controller))
	{
		ECRController->SetIsAutoRunning(false);
	}

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UECRHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UECRHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * ECRHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * ECRHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UECRHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (!bMovementInputEnabled)
	{
		return;
	}

	if (AECRCharacter* Character = GetPawn<AECRCharacter>())
	{
		Character->ToggleCrouch();
	}
}

void UECRHeroComponent::Input_AutoRun(const FInputActionValue& InputActionValue)
{
	if (!bMovementInputEnabled)
	{
		return;
	}

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (AECRPlayerController* Controller = Cast<AECRPlayerController>(Pawn->GetController()))
		{
			// Toggle auto running
			Controller->SetIsAutoRunning(!Controller->GetIsAutoRunning());
		}
	}
}

TSubclassOf<UECRCameraMode> UECRHeroComponent::DetermineCameraMode() const
{
	if (AbilityCameraMode)
	{
		return AbilityCameraMode;
	}

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return nullptr;
	}

	if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (const UECRPawnData* PawnData = PawnExtComp->GetPawnData<UECRPawnData>())
		{
			return PawnData->DefaultCameraMode;
		}
	}

	return nullptr;
}

void UECRHeroComponent::SetAbilityCameraMode(TSubclassOf<UECRCameraMode> CameraMode,
                                             const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (CameraMode)
	{
		AbilityCameraMode = CameraMode;
		AbilityCameraModeOwningSpecHandle = OwningSpecHandle;
	}
}

void UECRHeroComponent::ClearAbilityCameraMode(const FGameplayAbilitySpecHandle& OwningSpecHandle)
{
	if (AbilityCameraModeOwningSpecHandle == OwningSpecHandle)
	{
		AbilityCameraMode = nullptr;
		AbilityCameraModeOwningSpecHandle = FGameplayAbilitySpecHandle();
	}
}
