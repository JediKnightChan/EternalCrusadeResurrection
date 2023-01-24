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

	/** Storage for display names passed via map parameters */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TMap<AController*, FString> ControllersToDisplayNames;

protected:
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	                              const FString& Options, const FString& Portal) override;

public:
	AECRGameMode();

	// Agnostic version of PlayerCanRestart that can be used for both player bots and players
	virtual bool ControllerCanRestart(AController* Controller);
};
