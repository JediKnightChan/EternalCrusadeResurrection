// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Interaction/Tasks/AbilityTask_WaitForInteractableTargets.h"
#include "GameFramework/Actor.h"
#include "Physics/ECRCollisionChannels.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Interaction/InteractionStatics.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

UAbilityTask_WaitForInteractableTargets::UAbilityTask_WaitForInteractableTargets(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_WaitForInteractableTargets::LineOrSweepTrace(FHitResult& OutHitResult, const UWorld* World,
                                                               const FVector& Start, const FVector& End,
                                                               float SweepRadius,
                                                               const FCollisionQueryParams Params) const
{
	check(World);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;

	if (SweepRadius > 0)
	{
		World->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECR_TraceChannel_Interaction,
		                           FCollisionShape::MakeSphere(SweepRadius), Params);
	}
	else
	{
		World->LineTraceMultiByChannel(HitResults, Start, End, ECR_TraceChannel_Interaction, Params);
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

	APawn* const AvatarPawn = Cast<APawn>(Ability->GetAvatarActorFromActorInfo());
	if (AvatarPawn && AvatarPawn->Controller)
	{
		TObjectPtr<AController> PC = AvatarPawn->Controller;

		FVector ViewStart;
		FRotator ViewRot;
		PC->GetPlayerViewPoint(ViewStart, ViewRot);

		const FVector ViewDir = ViewRot.Vector();
		FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

		ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

		FHitResult HitResult;
		LineOrSweepTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, SweepRadius, Params);

		const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <=
			(
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

	GrantAbilitiesToAbilitySystem(InteractQuery, InteractableTargets);

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
					Option.TargetAbilitySystem = AbilitySystemComponent.Get();
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

void UAbilityTask_WaitForInteractableTargets::GrantAbilitiesToAbilitySystem(const FInteractionQuery& InteractQuery,
                                                                            const TArray<TScriptInterface<
	                                                                            IInteractableTarget>>&
                                                                            InteractableTargets)
{
	// Removing abilities queued for removal
	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;
	AbilitiesToRemove.GetKeys(HandlesToRemove);

	for (FGameplayAbilitySpecHandle Handle : HandlesToRemove)
	{
		FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec)
		{
			if (!Spec->IsActive())
			{
				AbilitySystemComponent->ClearAbility(Handle);
				FObjectKey ObjectKey = AbilitiesToRemove.FindRef(Handle);
				AbilitiesToRemove.Remove(Handle);
				InteractionAbilityCache.Remove(ObjectKey);
			}
		}
		else
		{
			FObjectKey ObjectKey = AbilitiesToRemove.FindRef(Handle);
			AbilitiesToRemove.Remove(Handle);
			InteractionAbilityCache.Remove(ObjectKey);
		}
	}

	// Gathering options
	TArray<FInteractionOption> Options;
	for (const TScriptInterface<IInteractableTarget>& InteractiveTarget : InteractableTargets)
	{
		FInteractionOptionBuilder InteractionBuilder(InteractiveTarget, Options);
		InteractiveTarget->GatherInteractionOptions(InteractQuery, InteractionBuilder);
	}

	// Queueing ability specs from options that disappeared for remove
	for (FInteractionOption& LastUpdateOption : LastUpdateOptions)
	{
		bool bKeepLastUpdateOptionAbility = false;
		for (FInteractionOption& NewOption : Options)
		{
			if (NewOption.InteractionAbilityToGrant == LastUpdateOption.InteractionAbilityToGrant)
			{
				bKeepLastUpdateOptionAbility = true;
			}
		}
		if (!bKeepLastUpdateOptionAbility)
		{
			FObjectKey ObjectKey(LastUpdateOption.InteractionAbilityToGrant);
			FGameplayAbilitySpecHandle SpecHandle = InteractionAbilityCache.FindRef(ObjectKey);

			// Queueing for remove
			AbilitiesToRemove.Add(SpecHandle, ObjectKey);

			// Removing mapping context if present
			APlayerController* Controller = Ability->GetActorInfo().PlayerController.Get();
			if (Controller)
			{
				if (const ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer())
				{
					if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<
						UEnhancedInputLocalPlayerSubsystem>())
					{
						Subsystem->RemoveMappingContext(LastUpdateOption.MappingContext);
					}
				}
			}
		}
	}

	// Check if any of the options need to grant the ability to the user before they can be used.
	for (FInteractionOption& Option : Options)
	{
		if (Option.InteractionAbilityToGrant)
		{
			// Grant the ability to the GAS, otherwise it won't be able to do whatever the interaction is.
			FObjectKey ObjectKey(Option.InteractionAbilityToGrant);
			if (!InteractionAbilityCache.Find(ObjectKey))
			{
				FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1);
				Spec.SourceObject = Option.AbilitySource ? Option.AbilitySource : this;
				if (Option.InputTag.IsValid())
				{
					Spec.DynamicAbilityTags.AddTag(Option.InputTag);
				}

				APlayerController* Controller = Ability->GetActorInfo().PlayerController.Get();

				if (Controller)
				{
					if (const ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer())
					{
						if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<
							UEnhancedInputLocalPlayerSubsystem>())
						{
							Subsystem->AddMappingContext(Option.MappingContext, Option.MappingContextPriority);
						}
					}
				}

				if (AbilitySystemComponent->IsOwnerActorAuthoritative())
				{
					FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
					InteractionAbilityCache.Add(ObjectKey, Handle);
				}
				else
				{
					InteractionAbilityCache.Add(ObjectKey, FGameplayAbilitySpecHandle{});
				}
			}
		}
	}

	LastUpdateOptions = Options;
}
