#include "CharacterOptimizerComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UCharacterOptimizerComponent::UCharacterOptimizerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	bAutoRegister = true;
	bAutoActivate = true;

	bOptimizationEnabled = true;
	NotInViewportDistance = 2000.f;
	ZeroWaveSettings = FCharOptimizationSettings{0.0f, 0.0f, true};
	FirstWaveDistance = 5000.0f;
	FirstWaveSettings = FCharOptimizationSettings{0.04f, 0.0f, false};
	SecondWaveDistance = 15000.0f;
	SecondWaveSettings = FCharOptimizationSettings{0.1, 0.0f, false};
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

				FVector2D ScreenLocation;
				PlayerController->ProjectWorldLocationToScreen(Char->GetActorLocation(), ScreenLocation);

				bool bIsInViewport;
				if (const UGameViewportClient* GameViewportClient = GetWorld()->GetGameViewport())
				{
					FVector2D ViewportSize;
					GameViewportClient->GetViewportSize(ViewportSize);
					bIsInViewport = ScreenLocation.X > 0 && ScreenLocation.Y > 0 && ScreenLocation.X < ViewportSize.X
						&& ScreenLocation.Y < ViewportSize.Y;
				}
				else
				{
					bIsInViewport = true;
				}

				const double VectorDistance = UKismetMathLibrary::Vector_Distance(Loc, Char->GetActorLocation());

				FCharOptimizationSettings Settings;
				if (Char->IsLocallyControlled())
				{
					Settings = ZeroWaveSettings;
				}
				else if (!bIsInViewport && VectorDistance >= NotInViewportDistance)
				{
					Settings = SecondWaveSettings;
				}
				else
				{
					if (VectorDistance < FirstWaveDistance)
					{
						Settings = ZeroWaveSettings;
					}
					else if (VectorDistance < SecondWaveDistance)
					{
						Settings = FirstWaveSettings;
					}
					else
					{
						Settings = SecondWaveSettings;
					}
				}
				ApplyOptimizationSettingsToChar(Settings);
			}
		}
	}
}
