// Copyleft: All rights reversed


#include "Gameplay/ECRGameState.h"

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

float AECRGameState::GetGlobalHealingMultiplier_Implementation(const UAbilitySystemComponent* SourceAsc,
	const UAbilitySystemComponent* TargetAsc)
{
	return 1.0f;
}

float AECRGameState::GetGlobalDamageMultiplier_Implementation(const UAbilitySystemComponent* SourceAsc, const UAbilitySystemComponent* TargetAsc)
{
	return 1.0f;
}
