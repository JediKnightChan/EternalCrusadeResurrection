// Copyleft: All rights reversed


#include "ECRGameState.h"

void AECRGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	OnMatchStarted();
}

void AECRGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
	OnMatchEnded();
}

void AECRGameState::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
	OnMatchWaitingToStart();
}
