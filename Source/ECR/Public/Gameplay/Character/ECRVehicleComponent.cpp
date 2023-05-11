#include "ECRVehicleComponent.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "ECRPawnData.h"
#include "ECRPawnExtensionComponent.h"
#include "InputActionValue.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/Vehicles/ECRWheeledVehiclePawn.h"
#include "Input/ECRInputComponent.h"

UECRVehicleComponent::UECRVehicleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void UECRVehicleComponent::OnPawnReadyToInitialize()
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

	AECRWheeledVehiclePawn* WheeledPawn = GetPawn<AECRWheeledVehiclePawn>();
	check(WheeledPawn);

	const UECRPawnData* PawnData = nullptr;

	if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		PawnData = PawnExtComp->GetPawnData<UECRPawnData>();

		// Vehicles ASC live on pawn
		PawnExtComp->InitializeAbilitySystem(WheeledPawn->GetECRAbilitySystemComponent(), WheeledPawn);
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

void UECRVehicleComponent::BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig)
{
	Super::BindNativeActions(ECRIC, InputConfig);
}
