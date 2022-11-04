// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRGameMode.h"
#include "Gameplay/ECRCharacter.h"
#include "UObject/ConstructorHelpers.h"

AECRGameMode::AECRGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
