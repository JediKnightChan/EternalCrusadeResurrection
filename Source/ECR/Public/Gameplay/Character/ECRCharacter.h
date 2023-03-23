// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Character.h"
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


/**
 * FECRReplicatedAcceleration: Compressed representation of acceleration
 */
USTRUCT()
struct FECRReplicatedAcceleration
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 AccelXYRadians = 0; // Direction of XY accel component, quantized to represent [0, 2*pi]

	UPROPERTY()
	uint8 AccelXYMagnitude = 0; //Accel rate of XY component, quantized to represent [0, MaxAcceleration]

	UPROPERTY()
	int8 AccelZ = 0; // Raw Z accel rate component, quantized to represent [-MaxAcceleration, MaxAcceleration]
};


/**
 * AECRCharacter
 *
 *	The base character pawn class used by this project.
 *	Responsible for sending events to pawn components.
 *	New behavior should be added via pawn components when possible.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class AECRCharacter : public ACharacter, public IAbilitySystemInterface, public IGameplayCueInterface,
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

	void ToggleCrouch();

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	//~End of AActor interface

	// Interactions
	/** Blueprint implementable event to get interaction options (like reviving, executing in wounded state) */
	UFUNCTION(BlueprintImplementableEvent)
	TArray<FInteractionOption> GetInteractionOptions(const FInteractionQuery InteractQuery);

	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	//~End of IInteractableTarget interface
protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void InitializeGameplayTags();
	void GrantAbilitySets(TArray<UECRAbilitySet*> AbilitySets) const;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRPawnExtensionComponent* PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRCharacterHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	UECRCameraComponent* CameraComponent;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReplicatedAcceleration)
	FECRReplicatedAcceleration ReplicatedAcceleration;

	static const FName NAME_ECRAbilityReady;

	UPROPERTY(ReplicatedUsing = OnRep_PawnData, EditAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
	const UECRPawnData* PawnData;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ECR|Character", Meta = (AllowPrivateAccess = "true"))
	float StartedFallingTime;

private:
	UFUNCTION()
	void OnRep_PawnData();

	UFUNCTION()
	void OnRep_ReplicatedAcceleration();
};
