// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/Tasks/AbilityTask_WaitForInteractableTargets.h"
#include "GameFramework/Actor.h"
#include "Physics/ECRCollisionChannels.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Interaction/InteractionStatics.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"

UAbilityTask_WaitForInteractableTargets::UAbilityTask_WaitForInteractableTargets(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_WaitForInteractableTargets::LineOrSweepTrace(FHitResult& OutHitResult, const UWorld* World,
                                                               const FVector& Start, const FVector& End,
                                                               FName ProfileName, float SweepRadius,
                                                               const FCollisionQueryParams Params) const
{
	check(World);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;

	if (SweepRadius > 0)
	{
		World->SweepMultiByProfile(HitResults, Start, End, FQuat::Identity, ProfileName,
		                           FCollisionShape::MakeSphere(SweepRadius), Params);
	}
	else
	{
		World->LineTraceMultiByProfile(HitResults, Start, End, ProfileName, Params);
	}

	OutHitResult.TraceStart = Start;
	OutHitResult.TraceEnd = End;

	if (HitResults.Num() > 0)
	{
		OutHitResult = HitResults[0];
	}
}

void UAbilityTask_WaitForInteractableTargets::AimWithPlayerController(const AActor* InSourceActor,
                                                                      FCollisionQueryParams Params,
                                                                      const FVector& TraceStart, float MaxRange,
                                                                      float SweepRadius, FVector& OutTraceEnd,
                                                                      bool bIgnorePitch) const
{
	if (!Ability) // Server and launching client only
	{
		return;
	}

	//@TODO: Bots?
	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	check(PC);

	FVector ViewStart;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewStart, ViewRot);

	const FVector ViewDir = ViewRot.Vector();
	FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	FHitResult HitResult;
	LineOrSweepTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, TraceProfile.Name, SweepRadius, Params);

	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (
		MaxRange * MaxRange));

	const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero())
	{
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult)
	{
		FVector OriginalAimDir = (ViewEnd - TraceStart).GetSafeNormal();

		if (!OriginalAimDir.IsZero())
		{
			// Convert to angles and use original pitch
			const FRotator OriginalAimRot = OriginalAimDir.Rotation();

			FRotator AdjustedAimRot = AdjustedAimDir.Rotation();
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}

	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}

bool UAbilityTask_WaitForInteractableTargets::ClipCameraRayToAbilityRange(
	FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange,
	FVector& ClippedPosition)
{
	FVector CameraToCenter = AbilityCenter - CameraLocation;
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
	if (DotToCenter >= 0)
	//If this fails, we're pointed away from the center, but we might be inside the sphere and able to find a good exit point.
	{
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);
		if (DistanceSquared <= RadiusSquared)
		{
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;
			//Subtracting instead of adding will get the other intersection point
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);
			//Cam aim point clipped to range sphere
			return true;
		}
	}
	return false;
}

void UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions(const FInteractionQuery& InteractQuery,
                                                                        const TArray<TScriptInterface<
	                                                                        IInteractableTarget>>& InteractableTargets)
{
	TArray<FInteractionOption> NewOptions;

	for (const TScriptInterface<IInteractableTarget>& InteractiveTarget : InteractableTargets)
	{
		TArray<FInteractionOption> TempOptions;
		FInteractionOptionBuilder InteractionBuilder(InteractiveTarget, TempOptions);
		InteractiveTarget->GatherInteractionOptions(InteractQuery, InteractionBuilder);

		for (FInteractionOption& Option : TempOptions)
		{
			FGameplayAbilitySpec* InteractionAbilitySpec = nullptr;

			// if there is a handle an a target ability system, we're triggering the ability on the target.
			if (Option.TargetAbilitySystem && Option.TargetInteractionAbilityHandle.IsValid())
			{
				// Find the spec
				InteractionAbilitySpec = Option.TargetAbilitySystem->FindAbilitySpecFromHandle(
					Option.TargetInteractionAbilityHandle);
			}
			// If there's an interaction ability then we're activating it on ourselves.
			else if (Option.InteractionAbilityToGrant)
			{
				// Find the spec
				InteractionAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(
					Option.InteractionAbilityToGrant);

				if (InteractionAbilitySpec)
				{
					// update the option
					Option.TargetAbilitySystem = AbilitySystemComponent;
					Option.TargetInteractionAbilityHandle = InteractionAbilitySpec->Handle;
				}
			}

			if (InteractionAbilitySpec)
			{
				// Filter any options that we can't activate right now for whatever reason.
				if (InteractionAbilitySpec->Ability->CanActivateAbility(InteractionAbilitySpec->Handle,
				                                                        AbilitySystemComponent->AbilityActorInfo.Get()))
				{
					NewOptions.Add(Option);
				}
			}
		}
	}

	bool bOptionsChanged = false;
	if (NewOptions.Num() == CurrentOptions.Num())
	{
		NewOptions.Sort();

		for (int OptionIndex = 0; OptionIndex < NewOptions.Num(); OptionIndex++)
		{
			const FInteractionOption& NewOption = NewOptions[OptionIndex];
			const FInteractionOption& CurrentOption = CurrentOptions[OptionIndex];

			if (NewOption != CurrentOption)
			{
				bOptionsChanged = true;
				break;
			}
		}
	}
	else
	{
		bOptionsChanged = true;
	}

	if (bOptionsChanged)
	{
		CurrentOptions = NewOptions;
		InteractableObjectsChanged.Broadcast(CurrentOptions);
	}
}
