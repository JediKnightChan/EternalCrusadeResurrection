// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "Gameplay/Interaction/IInteractableTarget.h"

#include "ECRCharacter.generated.h"


class AECRPlayerController;
class AECRPlayerState;
class UECRAbilitySystemComponent;
class UAbilitySystemComponent;
class UECRPawnExtensionComponent;
class UECRCharacterHealthComponent;
class UECRCameraComponent;
class UECRPawnData;
class UECRAbilitySet;

/** The type we use to send FastShared movement updates. */
USTRUCT()
struct FSharedRepMovement
{
	GENERATED_BODY()

	FSharedRepMovement();

	bool FillForCharacter(ACharacter* Character);
	bool Equals(const FSharedRepMovement& Other, ACharacter* Character) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(Transient)
	FRepMovement RepMovement;

	UPROPERTY(Transient)
	float RepTimeStamp = 0.0f;

	UPROPERTY(Transient)
	uint8 RepMovementMode = 0;

	UPROPERTY(Transient)
	bool bProxyIsJumpForceApplied = false;

	UPROPERTY(Transient)
	bool bIsCrouched = false;
};

/** The minimal ASC data (for simulated proxies). */
USTRUCT()
struct ECR_API FMinimalASCState
{
	GENERATED_BODY()

	FMinimalASCState();

	bool FillForCharacter(AECRCharacter* Character);
	bool Equals(const FMinimalASCState& Other) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	UPROPERTY(Transient)
	uint16 GameplayTagsBitMask = 0;

	UPROPERTY(Transient)
	uint8 Health = 0;

	UPROPERTY(Transient)
	uint8 Shield = 0;

	UPROPERTY(Transient)
	uint8 BleedingHealth = 0;

	UPROPERTY(Transient)
	float Armor = 0;
};

/**
 * AECRCharacter
 *
 *	The base character pawn class used by this project.
 *	Responsible for sending events to pawn components.
 *	New behavior should be added via pawn components when possible.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class AECRCharacter : public ACharacter, public IAbilitySystemInterface,
                      public IECRAbilitySystemReplicationProxyInterface, public IGameplayCueInterface,
                      public IGameplayTagAssetInterface, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AECRCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	UFUNCTION(BlueprintCallable, Category = "ECR|Character")
	AECRPlayerController* GetECRPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Character")
	AECRPlayerState* GetECRPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Character")
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void SetMainAnimLayer(TSubclassOf<UAnimInstance> InLayer);

	FORCEINLINE float GetGoingBackwardMultiplier() const { return GoingBackwardMultiplier; }
	FORCEINLINE float GetGoingSidewaysMultiplier() const { return GoingSidewaysMultiplier; }
	FORCEINLINE float GetOrientationToMovementOrientedRequirementAlpha() const
	{
		return OrientationToMovementOrientedRequirementAlpha;
	}

	/** Function for dirty fix of Root Motion Montages desync for sim proxies: each tick, when playing root motion,
	 * we'll check if distance from this transform exceeds threshold, then correct it */
	UFUNCTION(BlueprintPure)
	FORCEINLINE FTransform GetLatestReplicatedTransform() const
	{
		return FTransform{GetReplicatedMovement().Rotation, GetReplicatedMovement().Location};
	}

	void ToggleCrouch();

	UFUNCTION(BlueprintCallable)
	void GrantAbilitySets(TArray<UECRAbilitySet*> AbilitySets) const;

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	virtual void GetReplicatedCustomConditionState(FCustomPropertyConditionState& OutActiveState) const override;
	//~End of AActor interface

	// Begin: IAbilitySystemReplicationProxyInterface ~~ 
	virtual void ForceReplication() override;

	// Periodic effects should not trigger multicast for burst cues every period
	virtual void Call_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpecForRPC Spec, FPredictionKey PredictionKey) override;

	// Don't call multicasts for added gameplay cues, we use rep array ActiveGameplayCues on char

	virtual void Call_InvokeGameplayCueAdded(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext) override;
	virtual void Call_InvokeGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters Parameters) override;
	virtual void Call_InvokeGameplayCueAddedAndWhileActive_FromSpec(const FGameplayEffectSpecForRPC& Spec, FPredictionKey PredictionKey) override;
	virtual void Call_InvokeGameplayCueAddedAndWhileActive_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters) override;
	virtual void Call_InvokeGameplayCuesAddedAndWhileActive_WithParams(const FGameplayTagContainer GameplayCueTags, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters) override;

	// Actual multicasts for gameplay cues

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpecForRPC Spec,
	                                                             FPredictionKey PredictionKey) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueExecuted(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey,
	                                                    FGameplayEffectContextHandle EffectContext) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCuesExecuted(const FGameplayTagContainer GameplayCueTags,
	                                                     FPredictionKey PredictionKey,
	                                                     FGameplayEffectContextHandle EffectContext) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueExecuted_WithParams(const FGameplayTag GameplayCueTag,
	                                                               FPredictionKey PredictionKey,
	                                                               FGameplayCueParameters
	                                                               GameplayCueParameters) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCuesExecuted_WithParams(const FGameplayTagContainer GameplayCueTags,
	                                                                FPredictionKey PredictionKey,
	                                                                FGameplayCueParameters
	                                                                GameplayCueParameters) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueAdded(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey,
	                                                 FGameplayEffectContextHandle EffectContext) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag,
	                                                            FPredictionKey PredictionKey,
	                                                            FGameplayCueParameters Parameters) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueAddedAndWhileActive_FromSpec(
		const FGameplayEffectSpecForRPC& Spec, FPredictionKey PredictionKey) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCueAddedAndWhileActive_WithParams(
		const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey,
		FGameplayCueParameters GameplayCueParameters) override;

	UFUNCTION(NetMulticast, unreliable)
	virtual void NetMulticast_InvokeGameplayCuesAddedAndWhileActive_WithParams(
		const FGameplayTagContainer GameplayCueTags, FPredictionKey PredictionKey,
		FGameplayCueParameters GameplayCueParameters) override;

	virtual void Call_ReliableGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters Parameters) override;
	virtual void Call_ReliableGameplayCueRemoved(const FGameplayTag GameplayCueTag) override;

	virtual FGameplayAbilityRepAnimMontage& Call_GetRepAnimMontageInfo_Mutable() override;

	UFUNCTION()
	virtual void Call_OnRep_ReplicatedAnimMontage() override;
	// End: IAbilitySystemReplicationProxyInterface ~~

	// Interactions
	/** Blueprint implementable event to get interaction options (like reviving, executing in wounded state) */
	UFUNCTION(BlueprintImplementableEvent)
	TArray<FInteractionOption> GetInteractionOptions(const FInteractionQuery InteractQuery);

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	//~End of IInteractableTarget interface

	/** RPCs that is called on frames when default property replication is skipped. This replicates a single movement update to everyone. */
	UFUNCTION(NetMulticast, unreliable)
	void FastSharedMovementReplication(const FSharedRepMovement& SharedRepMovement);

	// Last FSharedRepMovement we sent, to avoid sending repeatedly.
	FSharedRepMovement LastSharedReplication;
	// Last time we performed a shared rep send
	float LastSharedReplicationTimestamp = 0.0f;

	// Last FMinimalASCState we sent, to avoid sending repeatedly.
	FMinimalASCState LastSharedAscState;

	// Whether root motion ended in this frame, required to fix RM replication engine issue
	bool bWasRootMotionPreviouslyActive;

	virtual bool UpdateSharedReplication();

protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void InitializeGameplayTags();

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	// Called when the death sequence for the character has completed
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	// Called when player state changed
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnPlayerStateChanged"))
	void K2_OnPlayerStateChanged();

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual bool CanJumpInternal_Implementation() const override;

	void InitPawnDataAndAbilities();

private:
	// The ability system component sub-object used by vehicles.
	UPROPERTY(VisibleAnywhere, Category = "ECR|Vehicle")
	UECRAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRPawnExtensionComponent* PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRCharacterHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRCameraComponent* CameraComponent;

	static const FName NAME_ECRAbilityReady;

	/** Information about the pawn derived from Lyra */
	UPROPERTY(ReplicatedUsing = OnRep_PawnData, EditAnywhere, BlueprintReadWrite,
		meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
	const UECRPawnData* PawnData;

	/** Animation layer linked to character currently (changed with weapon) */
	UPROPERTY(ReplicatedUsing = OnRep_MainAnimLayer)
	TSubclassOf<UAnimInstance> MainAnimLayer;

	/** Minimal replicated ASC data for sim proxies */
	UPROPERTY(ReplicatedUsing = OnRep_MinimalAscState)
	FMinimalASCState MinimalAscState;

	/** List of all active gameplay cues, stored on char instead of ASC due to ASC not replicated to sim proxies */
	UPROPERTY(BlueprintReadOnly, Replicated, meta=(AllowPrivateAccess="true"))
	FActiveGameplayCueContainer ActiveGameplayCues;

	/** Replicated anim montage info for ASC */
	UPROPERTY(ReplicatedUsing = Call_OnRep_ReplicatedAnimMontage)
	FGameplayAbilityRepAnimMontage RepAnimMontageInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	float StartedFallingTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	float StartedFallingZ;

	/** Multiplier for input value when going backwards */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	float GoingBackwardMultiplier;

	/** Multiplier for input value when going left or right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	float GoingSidewaysMultiplier;

	/** When rotation is oriented to movement, defines slowness when rotated not in movement direction (in order to first rotate, then move) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	float OrientationToMovementOrientedRequirementAlpha;

private:
	UFUNCTION()
	void OnRep_PawnData();

	UFUNCTION()
	void OnRep_MainAnimLayer();

	UFUNCTION()
	void OnRep_MinimalAscState();
};
