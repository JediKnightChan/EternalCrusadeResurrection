#include "Gameplay/Camera/ECRCameraMode_Static.h"
#include "Curves/CurveVector.h"


void UECRCameraMode_Static::OnActivation()
{
	Super::OnActivation();
	TimeRunning = 0.0f;
	SavedControlRotation = GetPivotRotation();
}

void UECRCameraMode_Static::UpdateView(float DeltaTime)
{
	TimeRunning += DeltaTime;

	// Constructing actor transform
	const AActor* TargetActor = GetTargetActor();
	check(TargetActor);
	const FVector ActorLocation = TargetActor->GetActorLocation();
	const FRotator ActorRotation = TargetActor->GetActorRotation();
	const FTransform ActorTransform{{0, ActorRotation.Yaw, 0}, ActorLocation, {1, 1, 1}};

	// Constructing local camera transform from curves
	const FVector CameraLocalLocation = TimeLocationCurve->GetVectorValue(TimeRunning);
	const FVector CurveRotatorValue = TimeRotationCurve->GetVectorValue(TimeRunning);
	const FRotator CameraLocalRotation = FRotator{CurveRotatorValue.X, CurveRotatorValue.Y, CurveRotatorValue.Z};
	const FTransform CameraLocalTransform{CameraLocalRotation, CameraLocalLocation, {1, 1, 1}};

	// Setting global camera transform
	const FTransform CameraGlobalTransform = CameraLocalTransform * ActorTransform;
	View.Location = CameraGlobalTransform.GetLocation();
	View.Rotation = CameraGlobalTransform.Rotator();
	View.ControlRotation = SavedControlRotation;
	View.FieldOfView = FieldOfView;

	Super::UpdateView(DeltaTime);
}
