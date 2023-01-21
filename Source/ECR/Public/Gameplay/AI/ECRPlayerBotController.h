// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "ECRPlayerBotController.generated.h"

/**
 * AECRPlayerBotController
 *
 *	The controller class used by player bots in this project.
 */
UCLASS(Blueprintable)
class AECRPlayerBotController : public AModularAIController
{
	GENERATED_BODY()

public:
	AECRPlayerBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Attempts to restart this controller (e.g., to respawn it)
	void ServerRestartController();

	virtual void OnUnPossess() override;

protected:
	// Called when the player state is set or cleared
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

protected:	
	//~AController interface
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

private:
	UPROPERTY()
	APlayerState* LastSeenPlayerState;
};
