// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/ECRAnimInstance.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/Character/ECRCharacterMovementComponent.h"


UECRAnimInstance::UECRAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UECRAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* ASC)
{
	check(ASC);

	GameplayTagPropertyMap.Initialize(this, ASC);
}

#if WITH_EDITOR
EDataValidationResult UECRAnimInstance::IsDataValid(TArray<FText>& ValidationErrors)
{
	Super::IsDataValid(ValidationErrors);

	GameplayTagPropertyMap.IsDataValid(this, ValidationErrors);

	return ((ValidationErrors.Num() > 0) ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}
#endif // WITH_EDITOR

void UECRAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor))
		{
			InitializeWithAbilitySystem(ASC);
		}
	}
}

void UECRAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const AECRCharacter* Character = Cast<AECRCharacter>(GetOwningActor());
	if (!Character)
	{
		return;
	}

	UECRCharacterMovementComponent* CharMoveComp = CastChecked<UECRCharacterMovementComponent>(
		Character->GetCharacterMovement());
	const FECRCharacterGroundInfo& GroundInfo = CharMoveComp->GetGroundInfo();
	GroundDistance = GroundInfo.GroundDistance;
}

void UECRAnimInstance::SetPawnUseControllerRotationYaw(APawn* Pawn,
                                                       const bool bUseControllerRotationYaw)
{
	if (Pawn)
	{
		Pawn->bUseControllerRotationYaw = bUseControllerRotationYaw;
	}
}
