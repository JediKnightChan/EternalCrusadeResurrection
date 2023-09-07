#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"

#include "ExtendablePawn.generated.h"


/**
 * AExtendablePawn
 *
 *	The base pawn class used by this project.
 *	Has PawnExtensionComponent that helps to use Enhanced Input subsystem, etc.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base simple pawn class used by this project."))
class AExtendablePawn : public APawn
{
	GENERATED_BODY()

public:
	AExtendablePawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRPawnExtensionComponent* PawnExtComponent;
};
