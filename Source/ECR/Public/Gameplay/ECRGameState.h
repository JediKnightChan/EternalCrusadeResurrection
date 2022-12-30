// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ECRGameState.generated.h"

class UECRAbilitySet;

/**
 * 
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API AECRGameState : public AGameState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TArray<UECRAbilitySet*> CommonCharacterAbilitySets;

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual void HandleMatchIsWaitingToStart() override;

	/** Handle match end in blueprints */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMatchWaitingToStart();

	/** Handle match start in blueprints */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMatchStarted();

	/** Handle match end in blueprints */
	UFUNCTION(BlueprintImplementableEvent)
	void OnMatchEnded();

public:
	/** Get CommonCharacterAbilitySets */
	FORCEINLINE TArray<UECRAbilitySet*> GetCommonCharacterAbilitySets() { return CommonCharacterAbilitySets; }
};
