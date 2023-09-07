#include "ExtendablePawn.h"

#include "Gameplay/Character/ECRPawnExtensionComponent.h"

AExtendablePawn::AExtendablePawn(const FObjectInitializer& ObjectInitializer)
{
	PawnExtComponent = CreateDefaultSubobject<UECRPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
}

void AExtendablePawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	PawnExtComponent->HandleControllerChanged();
}

void AExtendablePawn::UnPossessed()
{
	Super::UnPossessed();

	PawnExtComponent->HandleControllerChanged();
}

void AExtendablePawn::OnRep_Controller()
{
	Super::OnRep_Controller();

	PawnExtComponent->HandleControllerChanged();
}

void AExtendablePawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	PawnExtComponent->HandlePlayerStateReplicated();
}

void AExtendablePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PawnExtComponent->SetupPlayerInputComponent();
}
