// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonPlayerController.h"
#include "CommonLocalPlayer.h"

ACommonPlayerController::ACommonPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ACommonPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(Player))
	{
		LocalPlayer->OnPlayerControllerSet.Broadcast(LocalPlayer, this);

		if (PlayerState)
		{
			LocalPlayer->OnPlayerStateSet.Broadcast(LocalPlayer, PlayerState);
		}
	}
}

void ACommonPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(Player))
	{
		LocalPlayer->OnPlayerPawnSet.Broadcast(LocalPlayer, InPawn);
	}
}

void ACommonPlayerController::OnPossess(APawn* APawn)
{
	Super::OnPossess(APawn);
	
	if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(Player))
	{
		LocalPlayer->OnPlayerPawnSet.Broadcast(LocalPlayer, APawn);
	}
}

void ACommonPlayerController::OnUnPossess()
{
	Super::OnUnPossess();

	if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(Player))
	{
		LocalPlayer->OnPlayerPawnSet.Broadcast(LocalPlayer, nullptr);
	}
}

void ACommonPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (PlayerState)
	{
		if (UCommonLocalPlayer* LocalPlayer = Cast<UCommonLocalPlayer>(Player))
		{
			LocalPlayer->OnPlayerStateSet.Broadcast(LocalPlayer, PlayerState);
		}
	}
}