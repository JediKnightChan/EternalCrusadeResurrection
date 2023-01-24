// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SingleLineTrace.h"
#include "GameFramework/Actor.h"
#include "Physics/ECRCollisionChannels.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Interaction/InteractionStatics.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "AbilitySystemComponent.h"
#include "TimerManager.h"

UAbilityTask_WaitForInteractableTargets_SingleLineTrace::UAbilityTask_WaitForInteractableTargets_SingleLineTrace(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilityTask_WaitForInteractableTargets_SingleLineTrace*
UAbilityTask_WaitForInteractableTargets_SingleLineTrace::WaitForInteractableTargets_SingleLineTrace(
	UGameplayAbility* OwningAbility, FInteractionQuery InteractionQuery,
	FGameplayAbilityTargetingLocationInfo StartLocation, float InteractionScanRange, float InteractionScanRate,
	float SweepRadius, bool bShowDebug)
{
	UAbilityTask_WaitForInteractableTargets_SingleLineTrace* MyObj = NewAbilityTask<
		UAbilityTask_WaitForInteractableTargets_SingleLineTrace>(OwningAbility);
	MyObj->InteractionScanRange = InteractionScanRange;
	MyObj->InteractionScanRate = InteractionScanRate;
	MyObj->SweepRadius = SweepRadius;
	MyObj->StartLocation = StartLocation;
	MyObj->InteractionQuery = InteractionQuery;
	MyObj->bShowDebug = bShowDebug;

	return MyObj;
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::Activate()
{
	SetWaitingOnAvatar();

	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::PerformTrace, InteractionScanRate, true);
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::OnDestroy(bool AbilityEnded)
{
	Super::OnDestroy(AbilityEnded);

	UWorld* World = GetWorld();
	World->GetTimerManager().ClearTimer(TimerHandle);
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::PerformTrace()
{
	AActor* AvatarActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	UWorld* World = GetWorld();

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	const bool bTraceComplex = false;
	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(UAbilityTask_WaitForInteractableTargets_SingleLineTrace), bTraceComplex);
	Params.AddIgnoredActors(ActorsToIgnore);

	FVector TraceStart = StartLocation.GetTargetingTransform().GetLocation();
	FVector TraceEnd;
	AimWithPlayerController(AvatarActor, Params, TraceStart, InteractionScanRange, SweepRadius, OUT TraceEnd);

	FHitResult OutHitResult;
	// Trace
	LineOrSweepTrace(OutHitResult, World, TraceStart, TraceEnd, SweepRadius, Params);

	TArray<TScriptInterface<IInteractableTarget>> InteractableTargets;
	UInteractionStatics::AppendInteractableTargetsFromHitResult(OutHitResult, InteractableTargets);

	UpdateInteractableOptions(InteractionQuery, InteractableTargets);

#if ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		FColor DebugColor = OutHitResult.bBlockingHit ? FColor::Red : FColor::Green;
		FVector DrawTraceEnd;

		if (OutHitResult.bBlockingHit)
		{
			DrawTraceEnd = OutHitResult.Location;
			DrawDebugSphere(World, OutHitResult.Location, 5 + SweepRadius, 16, DebugColor, false, InteractionScanRate);
		}
		else
		{
			DrawTraceEnd = TraceEnd;
			DrawDebugLine(World, TraceStart, TraceEnd, DebugColor, false, InteractionScanRate);
		}

		if (SweepRadius > 0)
		{
			DrawDebugCylinder(World, TraceStart, DrawTraceEnd, SweepRadius, 32, DebugColor, false, InteractionScanRate);
		}
		else
		{
			DrawDebugLine(World, TraceStart, DrawTraceEnd, DebugColor, false, InteractionScanRate);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}
