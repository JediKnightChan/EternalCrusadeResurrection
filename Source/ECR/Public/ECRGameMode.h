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
	
protected:
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	                              const FString& Options, const FString& Portal) override;
public:
	AECRGameMode();
};



