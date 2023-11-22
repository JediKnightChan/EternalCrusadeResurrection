#include "ECRHeroComponent.h"

#include "ECRCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Input/ECRInputComponent.h"
#include "Kismet/KismetMathLibrary.h"


UECRHeroComponent::UECRHeroComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UECRHeroComponent::BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig)
{
	Super::BindNativeActions(ECRIC, InputConfig);

	const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();
	ECRIC->BindNativeAction(InputConfig, GameplayTags.InputTag_Move, ETriggerEvent::Triggered, this,
	                        &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
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
	const FVector PawnForwardVector = Pawn ? Pawn->GetActorForwardVector() : FVector::Zero();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	if (AECRPlayerController* ECRController = Cast<AECRPlayerController>(Controller))
	{
		ECRController->SetIsAutoRunning(false);
	}

	float BackwardsMultiplier = 1.0f;
	float SidewaysMultiplier = 1.0f;
	float OrientationToMovementOrientedRequirementAlpha = 0.0f;
	bool OrientsToMovement = false;

	if (AECRCharacter* Character = Cast<AECRCharacter>(Pawn))
	{
		BackwardsMultiplier = Character->GetGoingBackwardMultiplier();
		SidewaysMultiplier = Character->GetGoingSidewaysMultiplier();
		OrientationToMovementOrientedRequirementAlpha = Character->GetOrientationToMovementOrientedRequirementAlpha();

		if (UCharacterMovementComponent* CharMoveComp = Cast<UCharacterMovementComponent>(
			Character->GetMovementComponent()))
		{
			OrientsToMovement = CharMoveComp->bOrientRotationToMovement;
		}
	}

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			float XScaleValue = FMath::Clamp(Value.X, -1.0f, 1.0f);
			if (OrientsToMovement)
			{
				const float AngleToForwardCos = UKismetMathLibrary::Dot_VectorVector(
					PawnForwardVector, MovementDirection);
				XScaleValue = FMath::Lerp(XScaleValue, XScaleValue * FMath::Max(AngleToForwardCos, 0.01f),
				                          OrientationToMovementOrientedRequirementAlpha);
			}
			else
			{
				XScaleValue = XScaleValue * SidewaysMultiplier;
			}
			Pawn->AddMovementInput(MovementDirection, XScaleValue);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			float YScaleValue = FMath::Clamp(Value.Y, -1.0f, 1.0f);
			if (OrientsToMovement)
			{
				const float AngleToForwardCos = UKismetMathLibrary::Dot_VectorVector(
					PawnForwardVector, MovementDirection);
				YScaleValue = FMath::Lerp(YScaleValue, YScaleValue * FMath::Max(AngleToForwardCos, 0.01f),
										  OrientationToMovementOrientedRequirementAlpha);
			}
			else
			{
				if (YScaleValue < 0)
				{
					YScaleValue = YScaleValue * BackwardsMultiplier;
				}
			}
			Pawn->AddMovementInput(MovementDirection, YScaleValue);
		}
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
