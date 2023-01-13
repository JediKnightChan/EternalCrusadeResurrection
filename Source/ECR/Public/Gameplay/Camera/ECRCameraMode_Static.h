// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRCameraMode_PenetrationAvoidant.h"
#include "Curves/CurveFloat.h"

#include "ECRCameraMode_Static.generated.h"

class UCurveVector;

/**
 * UECRCameraMode_Static
 *
 *	A basic camera mode for camera not depending on user input, but on time since activation
 */
UCLASS(Abstract, Blueprintable)
class UECRCameraMode_Static : public UECRCameraMode_PenetrationAvoidant
{
	GENERATED_BODY()

public:
	virtual void OnActivation() override;

protected:
	virtual void UpdateView(float DeltaTime) override;

protected:
	float TimeRunning;
	FRotator SavedControlRotation;

	// Curve that defines location of camera using the time since camera mode activation to evaluate the curve.
	UPROPERTY(EditDefaultsOnly, Category = "Third Person")
	const UCurveVector* TimeLocationCurve;

	// Curve that defines rotation of camera using the time since camera mode activation to evaluate the curve.
	UPROPERTY(EditDefaultsOnly, Category = "Third Person")
	const UCurveVector* TimeRotationCurve;
};
