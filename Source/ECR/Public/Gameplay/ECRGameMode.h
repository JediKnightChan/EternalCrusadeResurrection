// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ECRGameMode.generated.h"

UCLASS(minimalapi)
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class AECRGameMode : public AGameMode
{
	GENERATED_BODY()

	/** Storage for options passed via join parameters */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<AController*, FString> ControllersToJoinOptions;

	/** Was overriden not to notify online session about match start, because it removes match then
	 * (even with join in progress set to true) */
	virtual void HandleMatchHasStarted() override;
protected:
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	                              const FString& Options, const FString& Portal) override;

public:
	AECRGameMode();

	// Agnostic version of PlayerCanRestart that can be used for both player bots and players
	virtual bool ControllerCanRestart(AController* Controller);
};
