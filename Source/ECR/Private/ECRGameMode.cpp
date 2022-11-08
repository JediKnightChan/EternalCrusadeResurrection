// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRGameMode.h"

#include "GameFramework/PlayerState.h"
#include "Gameplay/ECRCharacter.h"
#include "Gameplay/ECRMatchPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AECRGameMode::AECRGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

FString AECRGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
                                    const FString& Options, const FString& Portal)
{
	// Setting player display name according to options
	const FString PlayerName = UGameplayStatics::ParseOption(Options, TEXT("DisplayName"));
	UE_LOG(LogTemp, Warning, TEXT("New player name is %s"), *(PlayerName))
	if (AECRMatchPlayerState* PlayerState = Cast<AECRMatchPlayerState>(NewPlayerController->PlayerState))
	{
		PlayerState->DisplayName = PlayerName;
	}
	
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}
