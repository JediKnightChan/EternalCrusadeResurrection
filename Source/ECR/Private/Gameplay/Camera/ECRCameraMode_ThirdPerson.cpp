// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRCameraMode_ThirdPerson.h"
#include "Curves/CurveVector.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/Player/ECRPlayerController.h"


UECRCameraMode_ThirdPerson::UECRCameraMode_ThirdPerson()
{
	TargetOffsetCurve = nullptr;
}

void UECRCameraMode_ThirdPerson::UpdateView(float DeltaTime)
{
	UpdateForTarget(DeltaTime);
	UpdateCrouchOffset(DeltaTime);

	AActor* TargetActor = GetTargetActor();
	APawn* TargetPawn = Cast<APawn>(TargetActor);
	AController* TargetController = TargetPawn ? TargetPawn->GetController() : nullptr;

	CurrentPivotLocation = FMath::VInterpTo(CurrentPivotLocation, GetPivotLocation(), DeltaTime, 20.f);
	CurrentPivotRotation = FMath::RInterpTo(CurrentPivotRotation, GetPivotRotation(), DeltaTime, 20.f);

	FVector PivotLocation = (!TargetController ? CurrentPivotLocation : GetPivotLocation()) + CurrentCrouchOffset;
	FRotator PivotRotation = (!TargetController ? CurrentPivotRotation : GetPivotRotation());

	PivotRotation.Pitch = FMath::ClampAngle(PivotRotation.Pitch, ViewPitchMin, ViewPitchMax);

	View.Location = PivotLocation;
	View.Rotation = PivotRotation;
	View.ControlRotation = View.Rotation;
	View.FieldOfView = FieldOfView;

	// Apply third person offset using pitch.
	if (!bUseRuntimeFloatCurves)
	{
		if (TargetOffsetCurve)
		{
			FVector TargetOffset = TargetOffsetCurve->GetVectorValue(PivotRotation.Pitch);

			if (const APawn* Pawn = Cast<APawn>(GetTargetActor()))
			{
				if (const AECRPlayerController* Controller = Cast<AECRPlayerController>(Pawn->GetController()))
				{
					// Apply camera inversion
					if (Controller->bInvertCameraY)
					{
						TargetOffset.Y = -1 * TargetOffset.Y;
					}

					TargetOffset.X = Controller->CameraDistanceMultiplier * TargetOffset.X;
				}
			}

			View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
		}
	}
	else
	{
		FVector TargetOffset(0.0f);

		TargetOffset.X = TargetOffsetX.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Y = TargetOffsetY.GetRichCurveConst()->Eval(PivotRotation.Pitch);
		TargetOffset.Z = TargetOffsetZ.GetRichCurveConst()->Eval(PivotRotation.Pitch);

		if (const APawn* Pawn = Cast<APawn>(GetTargetActor()))
		{
			if (const AECRPlayerController* Controller = Cast<AECRPlayerController>(Pawn->GetController()))
			{
				// Apply camera inversion
				if (Controller->bInvertCameraY)
				{
					TargetOffset.Y = -1 * TargetOffset.Y;
				}

				TargetOffset.X = Controller->CameraDistanceMultiplier * TargetOffset.X;
			}
		}

		View.Location = PivotLocation + PivotRotation.RotateVector(TargetOffset);
	}

	Super::UpdateView(DeltaTime);
}

void UECRCameraMode_ThirdPerson::UpdateForTarget(float DeltaTime)
{
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(GetTargetActor()))
	{
		if (TargetCharacter->bIsCrouched)
		{
			const ACharacter* TargetCharacterCDO = TargetCharacter->GetClass()->GetDefaultObject<ACharacter>();
			const float CrouchedHeightAdjustment = TargetCharacterCDO->CrouchedEyeHeight - TargetCharacterCDO->
				BaseEyeHeight;

			SetTargetCrouchOffset(FVector(0.f, 0.f, CrouchedHeightAdjustment));

			return;
		}
	}

	SetTargetCrouchOffset(FVector::ZeroVector);
}

void UECRCameraMode_ThirdPerson::SetTargetCrouchOffset(FVector NewTargetOffset)
{
	CrouchOffsetBlendPct = 0.0f;
	InitialCrouchOffset = CurrentCrouchOffset;
	TargetCrouchOffset = NewTargetOffset;
}


void UECRCameraMode_ThirdPerson::UpdateCrouchOffset(float DeltaTime)
{
	if (CrouchOffsetBlendPct < 1.0f)
	{
		CrouchOffsetBlendPct = FMath::Min(CrouchOffsetBlendPct + DeltaTime * CrouchOffsetBlendMultiplier, 1.0f);
		CurrentCrouchOffset = FMath::InterpEaseInOut(InitialCrouchOffset, TargetCrouchOffset, CrouchOffsetBlendPct,
		                                             1.0f);
	}
	else
	{
		CurrentCrouchOffset = TargetCrouchOffset;
		CrouchOffsetBlendPct = 1.0f;
	}
}

FRotator UECRCameraMode_ThirdPerson::GetPivotRotation() const
{
	AActor* TargetActor = GetTargetActor();
	const APawn* TargetPawn = Cast<APawn>(TargetActor);
	if (AController* TargetController = TargetPawn ? TargetPawn->GetController() : nullptr)
	{
		return Super::GetPivotRotation();
	}
	return TargetPawn->GetBaseAimRotation();
}
