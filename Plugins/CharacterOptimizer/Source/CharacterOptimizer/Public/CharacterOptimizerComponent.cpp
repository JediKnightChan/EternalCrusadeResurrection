#include "CharacterOptimizerComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UCharacterOptimizerComponent::UCharacterOptimizerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoRegister = true;
	bAutoActivate = true;

	bOptimizationEnabled = true;
	ZeroWaveSettings = FCharOptimizationSettings{0.0f, 0.0f, true};
	FirstWaveDistance = 5000.0f;
	FirstWaveSettings = FCharOptimizationSettings{0.05, 0.0f, false};
	SecondWaveDistance = 10000.0f;
	SecondWaveSettings = FCharOptimizationSettings{0.1, 0.05f, false};
}

void UCharacterOptimizerComponent::OnRegister()
{
	Super::OnRegister();

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		Char = OwnerChar;
	}
}

void UCharacterOptimizerComponent::ApplyOptimizationSettingsToChar(FCharOptimizationSettings& Settings)
{
	if (Char)
	{
		if (UCharacterMovementComponent* CMC = Char->GetCharacterMovement())
		{
			if (CMC->GetComponentTickInterval() != Settings.CharMoveComponentTickInterval)
			{
				CMC->SetComponentTickInterval(Settings.CharMoveComponentTickInterval);
			}
		}

		if (USkeletalMeshComponent* SkeletalMeshComponent = Char->GetMesh())
		{
			SkeletalMeshComponent->SetCastShadow(Settings.bShadowsTurnedOn);
		}

		if (Char->GetActorTickInterval() != Settings.CharTickInterval)
		{
			Char->SetActorTickInterval(Settings.CharTickInterval);
		}
	}
}

void UCharacterOptimizerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Char && bOptimizationEnabled)
	{
		if (Char->GetNetMode() >= NM_Client || Char->GetNetMode() == NM_Standalone)
		{
			if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				FVector Loc;
				FRotator Rot;
				PlayerController->GetActorEyesViewPoint(Loc, Rot);

				const double VectorDistance = UKismetMathLibrary::Vector_Distance(Loc, Char->GetActorLocation());
				if (VectorDistance < FirstWaveDistance || Char->IsLocallyControlled())
				{
					ApplyOptimizationSettingsToChar(ZeroWaveSettings);
				}
				else if (VectorDistance < SecondWaveDistance)
				{
					// UE_LOG(LogTemp, Warning, TEXT("First wave"))
					ApplyOptimizationSettingsToChar(FirstWaveSettings);
				}
				else
				{
					// UE_LOG(LogTemp, Warning, TEXT("Second wave"))
					ApplyOptimizationSettingsToChar(SecondWaveSettings);
				}
			}
		}
	}
}
