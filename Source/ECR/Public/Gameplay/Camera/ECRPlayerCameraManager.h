// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ECRPlayerCameraManager.generated.h"


#define ECR_CAMERA_DEFAULT_FOV			(80.0f)
#define ECR_CAMERA_DEFAULT_PITCH_MIN	(-89.0f)
#define ECR_CAMERA_DEFAULT_PITCH_MAX	(89.0f)

class UECRUICameraManagerComponent;

/**
 * AECRPlayerCameraManager
 *
 *	The base player camera manager class used by this project.
 */
UCLASS(notplaceable, MinimalAPI)
class AECRPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:

	AECRPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	UECRUICameraManagerComponent* GetUICameraComponent() const;

protected:

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

private:
	/** The UI Camera Component, controls the camera when UI is doing something important that gameplay doesn't get priority over. */
	UPROPERTY(Transient)
	UECRUICameraManagerComponent* UICamera;
};
