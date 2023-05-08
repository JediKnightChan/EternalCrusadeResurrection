#include "ECRHeroComponent.h"

#include "ECRCharacter.h"
#include "EnhancedInputComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Input/ECRInputComponent.h"


UECRHeroComponent::UECRHeroComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UECRHeroComponent::BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig)
{
	Super::BindNativeActions(ECRIC, InputConfig);

	const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();
	ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Move, ETriggerEvent::Triggered, this,
	                        &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
	ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Crouch, ETriggerEvent::Triggered, this,
	                        &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
	ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_AutoRun, ETriggerEvent::Triggered, this,
	                        &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
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
