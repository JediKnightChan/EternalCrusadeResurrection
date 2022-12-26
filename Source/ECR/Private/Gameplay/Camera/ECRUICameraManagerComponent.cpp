// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Camera/ECRUICameraManagerComponent.h"
#include "EngineUtils.h"
#include "Algo/Transform.h"
#include "DrawDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SceneViewport.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/HUD.h"
#include "Engine/Engine.h"

#include "Gameplay/Camera/ECRCameraMode.h"
#include "Gameplay/Camera/ECRPlayerCameraManager.h"

UECRUICameraManagerComponent* UECRUICameraManagerComponent::GetComponent(APlayerController* PC)
{
	if (PC != nullptr)
	{
		if (AECRPlayerCameraManager* PCCamera = Cast<AECRPlayerCameraManager>(PC->PlayerCameraManager))
		{
			return PCCamera->GetUICameraComponent();
		}
	}

	return nullptr;
}

UECRUICameraManagerComponent::UECRUICameraManagerComponent()
{
	bWantsInitializeComponent = true;

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Register "showdebug" hook.
		if (!IsRunningDedicatedServer())
		{
			AHUD::OnShowDebugInfo.AddUObject(this, &ThisClass::OnShowDebugInfo);
		}
	}
}

void UECRUICameraManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UECRUICameraManagerComponent::SetViewTarget(AActor* InViewTarget, FViewTargetTransitionParams TransitionParams)
{
	TGuardValue<bool> UpdatingViewTargetGuard(bUpdatingViewTarget, true);

	ViewTarget = InViewTarget;
	CastChecked<AECRPlayerCameraManager>(GetOwner())->SetViewTarget(ViewTarget, TransitionParams);
}

bool UECRUICameraManagerComponent::NeedsToUpdateViewTarget() const
{
	return false;
}

void UECRUICameraManagerComponent::UpdateViewTarget(struct FTViewTarget& OutVT, float DeltaTime)
{
}

void UECRUICameraManagerComponent::OnShowDebugInfo(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
}