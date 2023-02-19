// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ECRTaggedActor.h"
#include "ECRPlayerOwnedTaggedActor.generated.h"

// Tagged actor with access to player state who is responsible for its creation
UCLASS()
class AECRPlayerOwnedTaggedActor : public AECRTaggedActor
{
	GENERATED_BODY()

	// Player state who is responsible for creation of this actor
	UPROPERTY(BlueprintReadOnly, Category=Actor, meta=(AllowPrivateAccess="true"))
	APlayerState* OwningPlayerState;

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnOwningPlayerStateChanged();

public:
	AECRPlayerOwnedTaggedActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Externally set player state responsible for creation of this actor */
	void SetPlayerState(APlayerState* NewState);
};
