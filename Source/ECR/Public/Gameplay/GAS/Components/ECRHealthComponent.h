// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameFrameworkComponent.h"
#include "ECRHealthComponent.generated.h"


class UECRAbilitySystemComponent;
class UECRHealthSet;
struct FGameplayEffectSpec;
struct FOnAttributeChangeData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FECRHealth_StatusEvent, AActor*, OwningActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FECRHealth_AttributeChanged, UECRHealthComponent*, HealthComponent, float,
                                              OldValue, float, NewValue, AActor*, Instigator);


/**
 * EECRDeathState
 *
 *	Defines current state of death.
 */
UENUM(BlueprintType)
enum class EECRDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};


/**
 * UECRHealthComponent
 *
 *	An actor component used to handle anything related to health.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class ECR_API UECRHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UECRHealthComponent(const FObjectInitializer& ObjectInitializer);

	static AActor* GetInstigatorFromAttrChangeData(const FOnAttributeChangeData& ChangeData);

	// Returns the health component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "ECR|Health")
	static UECRHealthComponent* FindHealthComponent(const AActor* Actor)
	{
		return (Actor ? Actor->FindComponentByClass<UECRHealthComponent>() : nullptr);
	}

	// Initialize the component using an ability system component.
	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	virtual void InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC);

	// Uninitialize the component, clearing any references to the ability system.
	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	virtual void UninitializeFromAbilitySystem();

	// Returns the current health value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	float GetHealth() const;

	// Returns the current maximum health value.
	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	float GetMaxHealth() const;

	// Returns the current health in the range [0.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Health")
	EECRDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "ECR|Health",
		Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return (DeathState > EECRDeathState::NotDead); }

	// Begins the death sequence for the owner.
	virtual void StartDeath();

	// Ends the death sequence for the owner.
	virtual void FinishDeath();
	

	// Applies enough damage to kill the owner.
	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

	// Gets enough damage to kill the owner.
	virtual float GetDamageToKill();
public:
	// Delegate fired when the health value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnHealthChanged;

	// Delegate fired when the max health value has changed.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_AttributeChanged OnMaxHealthChanged;

	// Delegate fired when the death sequence has started.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_StatusEvent OnDeathStarted;

	// Delegate fired when the death sequence has finished.
	UPROPERTY(BlueprintAssignable)
	FECRHealth_StatusEvent OnDeathFinished;

protected:
	virtual void OnUnregister() override;

	static float GetNormalizedAttributeValue(const float Value, const float MaxValue);

	virtual void ClearGameplayTags();

	virtual void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleReadyToDie(AActor* DamageInstigator, AActor* DamageCauser,
	                              const FGameplayEffectSpec& DamageEffectSpec, float DamageMagnitude);

	UFUNCTION()
	virtual void OnRep_DeathState(EECRDeathState OldDeathState);

protected:
	// Ability system used by this component.
	UPROPERTY()
	UECRAbilitySystemComponent* AbilitySystemComponent;

	// Health set used by this component.
	UPROPERTY()
	const UECRHealthSet* HealthSet;

	// Replicated state used to handle dying.
	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EECRDeathState DeathState;
};
