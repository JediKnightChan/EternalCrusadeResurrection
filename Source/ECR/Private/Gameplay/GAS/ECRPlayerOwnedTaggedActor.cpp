#include "Gameplay/GAS/ECRPlayerOwnedTaggedActor.h"

AECRPlayerOwnedTaggedActor::AECRPlayerOwnedTaggedActor(const FObjectInitializer& ObjectInitializer)
{
	OwningPlayerState = nullptr;
}

void AECRPlayerOwnedTaggedActor::SetPlayerState(APlayerState* NewState)
{
	OwningPlayerState = NewState;
	OnOwningPlayerStateChanged();
}
