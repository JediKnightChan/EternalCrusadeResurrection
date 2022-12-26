// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRPlayerCameraManager.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Engine/Canvas.h"
#include "Gameplay/Camera/ECRUICameraManagerComponent.h"
#include "GameFramework/PlayerController.h"

static FName UICameraComponentName(TEXT("UICamera"));

AECRPlayerCameraManager::AECRPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultFOV = ECR_CAMERA_DEFAULT_FOV;
	ViewPitchMin = ECR_CAMERA_DEFAULT_PITCH_MIN;
	ViewPitchMax = ECR_CAMERA_DEFAULT_PITCH_MAX;

	UICamera = CreateDefaultSubobject<UECRUICameraManagerComponent>(UICameraComponentName);
}

UECRUICameraManagerComponent* AECRPlayerCameraManager::GetUICameraComponent() const
{
	return UICamera;
}

void AECRPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// If the UI Camera is looking at something, let it have priority.
	if (UICamera->NeedsToUpdateViewTarget())
	{
		Super::UpdateViewTarget(OutVT, DeltaTime);
		UICamera->UpdateViewTarget(OutVT, DeltaTime);
		return;
	}

	Super::UpdateViewTarget(OutVT, DeltaTime);
}

void AECRPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	check(Canvas);

	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;

	DisplayDebugManager.SetFont(GEngine->GetSmallFont());
	DisplayDebugManager.SetDrawColor(FColor::Yellow);
	DisplayDebugManager.DrawString(FString::Printf(TEXT("ECRPlayerCameraManager: %s"), *GetNameSafe(this)));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

	const APawn* Pawn = (PCOwner ? PCOwner->GetPawn() : nullptr);

	if (const UECRCameraComponent* CameraComponent = UECRCameraComponent::FindCameraComponent(Pawn))
	{
		CameraComponent->DrawDebug(Canvas);
	}
}
