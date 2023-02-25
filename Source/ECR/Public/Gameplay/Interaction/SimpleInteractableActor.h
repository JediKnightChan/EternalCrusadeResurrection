// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Gameplay/Interaction/InteractionQuery.h"
#include "Gameplay/Interaction/IInteractableTarget.h"

#include "SimpleInteractableActor.generated.h"

// An actor that can provide interaction options
UCLASS()
class ASimpleInteractableActor : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	ASimpleInteractableActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Interactions

	/** Interaction options map */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TMap<FName, FInteractionOption> InteractionOptionsMap;

	/** Blueprint implementable event to get interaction options */
	UFUNCTION(BlueprintImplementableEvent)
	TArray<FInteractionOption> GetInteractionOptions(const FInteractionQuery InteractQuery);

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	//~End of IInteractableTarget interface
};
