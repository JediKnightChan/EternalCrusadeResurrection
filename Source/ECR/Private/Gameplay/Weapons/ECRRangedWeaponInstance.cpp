// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Camera/ECRCameraComponent.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Physics/PhysicalMaterialWithTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Weapon_SteadyAimingCamera, "ECR.Weapon.SteadyAimingCamera");

UECRRangedWeaponInstance::UECRRangedWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeatToHeatPerShotCurve.EditorCurveData.AddKey(0.0f, 1.0f);
	HeatToCoolDownPerSecondCurve.EditorCurveData.AddKey(0.0f, 2.0f);
}

void UECRRangedWeaponInstance::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

#if WITH_EDITOR
void UECRRangedWeaponInstance::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateDebugVisualization();
}

void UECRRangedWeaponInstance::UpdateDebugVisualization()
{
	ComputeHeatRange(/*out*/ Debug_MinHeat, /*out*/ Debug_MaxHeat);
	ComputeSpreadRange(/*out*/ Debug_MinSpreadAngle, /*out*/ Debug_MaxSpreadAngle);
	Debug_CurrentHeat = CurrentHeat;
	Debug_CurrentSpreadAngle = CurrentSpreadAngle;
	Debug_CurrentSpreadAngleMultiplier = CurrentSpreadAngleMultiplier;
}
#endif

void UECRRangedWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	// Start heat in the start
	float MinHeatRange;
	float MaxHeatRange;
	ComputeHeatRange(/*out*/ MinHeatRange, /*out*/ MaxHeatRange);

	CurrentHeat = 0;
	if (APawn* Pawn = GetPawn())
	{
		if (UECREquipmentManagerComponent* Comp = Pawn->FindComponentByClass<UECREquipmentManagerComponent>())
		{
			CurrentHeat = Comp->SavedCurrentHeat.FindRef(
				UKismetSystemLibrary::GetClassDisplayName(UGameplayStatics::GetObjectClass(this)));
		}
	}
	UpdateFiringTime();

	// Derive spread
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat * HeatToSpreadMappingMultiplier);

	// Default the multipliers to 1x
	CurrentSpreadAngleMultiplier = 1.0f;
	StandingStillMultiplier = 1.0f;
	JumpFallMultiplier = 1.0f;
	CrouchingMultiplier = 1.0f;
}

void UECRRangedWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();
}

void UECRRangedWeaponInstance::Tick(float DeltaSeconds)
{
	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);

	const bool bMinSpread = UpdateSpread(DeltaSeconds);
	const bool bMinMultipliers = UpdateMultipliers(DeltaSeconds);

	bHasFirstShotAccuracy = bAllowFirstShotAccuracy && bMinMultipliers && bMinSpread;

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

void UECRRangedWeaponInstance::ComputeHeatRange(float& MinHeat, float& MaxHeat)
{
	float Min1;
	float Max1;
	HeatToHeatPerShotCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min1, /*out*/ Max1);

	float Min2;
	float Max2;
	HeatToCoolDownPerSecondCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min2, /*out*/ Max2);

	float Min3;
	float Max3;
	HeatToSpreadCurve.GetRichCurveConst()->GetTimeRange(/*out*/ Min3, /*out*/ Max3);

	MinHeat = FMath::Min(FMath::Min(Min1, Min2), Min3);
	MaxHeat = FMath::Max(FMath::Max(Max1, Max2), Max3);
}

void UECRRangedWeaponInstance::ComputeSpreadRange(float& MinSpread, float& MaxSpread)
{
	HeatToSpreadCurve.GetRichCurveConst()->GetValueRange(/*out*/ MinSpread, /*out*/ MaxSpread);
}

void UECRRangedWeaponInstance::AddSpread()
{
	// Sample the heat up curve
	const float HeatPerShot = HeatToHeatPerShotCurve.GetRichCurveConst()->Eval(CurrentHeat) * HeatPerShotMultiplier;
	CurrentHeat = ClampHeat(CurrentHeat + HeatPerShot);

	// Map the heat to the spread angle
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat * HeatToSpreadMappingMultiplier);

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

void UECRRangedWeaponInstance::OverrideHeat(float NewHeat)
{
	CurrentHeat = NewHeat;
}

void UECRRangedWeaponInstance::RemoveHeat(float DeltaHeat)
{
	CurrentHeat = ClampHeat(CurrentHeat - DeltaHeat);
	// Map the heat to the spread angle
	CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat * HeatToSpreadMappingMultiplier);

#if WITH_EDITOR
	UpdateDebugVisualization();
#endif
}

float UECRRangedWeaponInstance::GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags,
                                                       const FGameplayTagContainer* TargetTags) const
{
	const float DamageMultiplier = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(DamageNearDistance, DamageFarDistance),
		                /*OutputRange=*/ FVector2D(1.0f, DamageFarMultiplier),
		                /*Alpha=*/ Distance);
	return DamageMultiplier;
}

float UECRRangedWeaponInstance::GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial,
                                                               const FGameplayTagContainer* SourceTags,
                                                               const FGameplayTagContainer* TargetTags) const
{
	float CombinedMultiplier = 1.0f;
	if (const UPhysicalMaterialWithTags* PhysMatWithTags = Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
	{
		for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
		{
			if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
			{
				CombinedMultiplier *= *pTagMultiplier;
			}
		}
	}

	return CombinedMultiplier;
}

void UECRRangedWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// DOREPLIFETIME(ThisClass, CurrentHeat);
}

bool UECRRangedWeaponInstance::UpdateSpread(float DeltaSeconds)
{
	if (APawn* Pawn = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
		{
			if (ASC->HasMatchingGameplayTag(FECRGameplayTags::Get().Status_NoWeaponHeatRemove))
			{
				UpdateFiringTime();
			}
		}
	}

	const float TimeSinceFired = GetWorld()->TimeSince(TimeLastFired);

	if (TimeSinceFired > SpreadRecoveryCooldownDelay)
	{
		const float CooldownRate = HeatToCoolDownPerSecondCurve.GetRichCurveConst()->Eval(CurrentHeat);
		CurrentHeat = ClampHeat(CurrentHeat - (CooldownRate * DeltaSeconds));
		CurrentSpreadAngle = HeatToSpreadCurve.GetRichCurveConst()->Eval(CurrentHeat * HeatToSpreadMappingMultiplier);
	}

	float MinSpread;
	float MaxSpread;
	ComputeSpreadRange(/*out*/ MinSpread, /*out*/ MaxSpread);

	return FMath::IsNearlyEqual(CurrentSpreadAngle, MinSpread, KINDA_SMALL_NUMBER);
}

bool UECRRangedWeaponInstance::UpdateMultipliers(float DeltaSeconds)
{
	const float MultiplierNearlyEqualThreshold = 0.05f;

	APawn* Pawn = GetPawn();
	check(Pawn != nullptr);
	UCharacterMovementComponent* CharMovementComp = Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent());

	// See if we are standing still, and if so, smoothly apply the bonus
	const float PawnSpeed = Pawn->GetVelocity().Size();
	const float MovementTargetValue = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(StandingStillSpeedThreshold,
		                          StandingStillSpeedThreshold + StandingStillToMovingSpeedRange),
		                /*OutputRange=*/ FVector2D(SpreadAngleMultiplier_StandingStill, 1.0f),
		                /*Alpha=*/ PawnSpeed);
	StandingStillMultiplier = FMath::FInterpTo(StandingStillMultiplier, MovementTargetValue, DeltaSeconds,
	                                           TransitionRate_StandingStill);
	const bool bStandingStillMultiplierAtMin = FMath::IsNearlyEqual(StandingStillMultiplier,
	                                                                SpreadAngleMultiplier_StandingStill,
	                                                                SpreadAngleMultiplier_StandingStill * 0.1f);

	// See if we are crouching, and if so, smoothly apply the bonus
	const bool bIsCrouching = (CharMovementComp != nullptr) && CharMovementComp->IsCrouching();
	const float CrouchingTargetValue = bIsCrouching ? SpreadAngleMultiplier_Crouching : 1.0f;
	CrouchingMultiplier = FMath::FInterpTo(CrouchingMultiplier, CrouchingTargetValue, DeltaSeconds,
	                                       TransitionRate_Crouching);
	const bool bCrouchingMultiplierAtTarget = FMath::IsNearlyEqual(CrouchingMultiplier, CrouchingTargetValue,
	                                                               MultiplierNearlyEqualThreshold);

	// See if we are in the air (jumping/falling), and if so, smoothly apply the penalty
	const bool bIsJumpingOrFalling = (CharMovementComp != nullptr) && CharMovementComp->IsFalling();
	const float JumpFallTargetValue = bIsJumpingOrFalling ? SpreadAngleMultiplier_JumpingOrFalling : 1.0f;
	JumpFallMultiplier = FMath::FInterpTo(JumpFallMultiplier, JumpFallTargetValue, DeltaSeconds,
	                                      TransitionRate_JumpingOrFalling);
	const bool bJumpFallMultiplerIs1 = FMath::IsNearlyEqual(JumpFallMultiplier, 1.0f, MultiplierNearlyEqualThreshold);

	// Determine if we are aiming down sights, and apply the bonus based on how far into the camera transition we are
	float AimingAlpha = 0.0f;
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
	{
		if (ASC->HasMatchingGameplayTag(FECRGameplayTags::Get().Status_ADS))
		{
			AimingAlpha = 1.0f;
		}
	}

	const float AimingMultiplier = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(0.0f, 1.0f),
		                /*OutputRange=*/ FVector2D(1.0f, SpreadAngleMultiplier_Aiming),
		                /*Alpha=*/ AimingAlpha);
	const bool bAimingMultiplierAtTarget = FMath::IsNearlyEqual(AimingMultiplier, SpreadAngleMultiplier_Aiming,
	                                                            KINDA_SMALL_NUMBER);

	// Determine if we are bracing
	float BracingAlpha = 0.0f;
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
	{
		if (ASC->HasMatchingGameplayTag(FECRGameplayTags::Get().Status_Bracing))
		{
			BracingAlpha = 1.0f;
		}
	}

	const float BracingMultiplier = FMath::GetMappedRangeValueClamped(
		/*InputRange=*/ FVector2D(0.0f, 1.0f),
		                /*OutputRange=*/ FVector2D(1.0f, SpreadAngleMultiplier_Bracing),
		                /*Alpha=*/ BracingAlpha);
	const bool bBracingAlphaMultiplierAtTarget = FMath::IsNearlyEqual(BracingMultiplier, SpreadAngleMultiplier_Bracing,
	                                                                  KINDA_SMALL_NUMBER);

	// Combine all the multipliers
	const float CombinedMultiplier = AimingMultiplier * StandingStillMultiplier * CrouchingMultiplier *
		JumpFallMultiplier * BracingMultiplier * SpreadAngleMultiplier_Modifier;

	// Clamping spread angle multiplier
	CurrentSpreadAngleMultiplier = FMath::Clamp(CombinedMultiplier, SpreadAngleMultiplier_Min,
	                                            SpreadAngleMultiplier_Max);

	// need to handle these spread multipliers indicating we are not at min spread
	return bStandingStillMultiplierAtMin && bCrouchingMultiplierAtTarget && bJumpFallMultiplerIs1 &&
		bAimingMultiplierAtTarget && bBracingAlphaMultiplierAtTarget;
}
