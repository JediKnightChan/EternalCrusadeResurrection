// Copyleft: All rights reversed


#include "Gameplay/ECRMatchPlayerState.h"

#include "Net/UnrealNetwork.h"

void AECRMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AECRMatchPlayerState, DisplayName);
}
