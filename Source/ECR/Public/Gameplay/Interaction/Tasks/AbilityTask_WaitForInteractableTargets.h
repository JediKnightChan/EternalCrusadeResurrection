// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "Engine/CollisionProfile.h"
#include "Abilities/GameplayAbilityTargetDataFilter.h"
#include "Gameplay/Interaction/InteractionOption.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "AbilityTask_WaitForInteractableTargets.generated.h"

class AActor;
class UPrimitiveComponent;
class UGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractableObjectsChangedEvent, const TArray<FInteractionOption>&,
                                            InteractableOptions);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDebugStringChangedEvent);

UCLASS(Abstract)
class UAbilityTask_WaitForInteractableTargets : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FInteractableObjectsChangedEvent InteractableObjectsChanged;

	UPROPERTY(BlueprintAssignable)
	FDebugStringChangedEvent GrantingDebugStringChanged;

protected:
	void LineOrSweepTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End,
	                      float SweepRadius, const FCollisionQueryParams Params) const;

	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart,
	                             float MaxRange, float SweepRadius, FVector& OutTraceEnd,
	                             bool bIgnorePitch = false) const;

	static bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter,
	                                        float AbilityRange, FVector& ClippedPosition);

	void UpdateInteractableOptions(const FInteractionQuery& InteractQuery,
	                               const TArray<TScriptInterface<IInteractableTarget>>& InteractableTargets);

	void ServerGrantAbilitiesToAbilitySystem(const FInteractionQuery& InteractQuery,
	                                   const TArray<TScriptInterface<IInteractableTarget>>& InteractableTargets);
	void OwnerUpdateAbilities(const FInteractionQuery& InteractQuery,
	                          const TArray<TScriptInterface<IInteractableTarget>>& InteractableTargets);

	UFUNCTION(BlueprintCallable)
	void ClearCache();

	// Does the trace affect the aiming pitch
	bool bTraceAffectsAimPitch = true;

	TArray<FInteractionOption> CurrentOptions;

private:
	TMap<FObjectKey, FGameplayAbilitySpecHandle> ServerInteractionAbilityCache;

	TArray<FInteractionOption> OwnerLastUpdateOptions;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FString GrantingDebugString;
};
