#pragma once

#include "Gameplay/Pawn/ExtendablePawn.h"
#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"

#include "ExtendableGASPawn.generated.h"

class UECRAbilitySet;
class UECRAbilitySystemComponent;
class UECRSimpleVehicleHealthSet;
class UECRCharacterHealthComponent;
class UECRPawnData;

/**
 * AExtendableGASPawn
 *
 *	Pawn with GAS component.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "Extendable pawn with Ability System Component."))
class AExtendableGASPawn : public AExtendablePawn, public IAbilitySystemInterface,
                           public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AExtendableGASPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable)
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PossessedBy(AController* NewController) override;

protected:
	void GrantAbilitySets(const TArray<UECRAbilitySet*> AbilitySets) const;
	void InitPawnDataAndAbilities();

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnDeathFinished(AActor* OwningActor);

private:
	UPROPERTY(VisibleAnywhere)
	UECRAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Vehicle", Meta = (AllowPrivateAccess = "true"))
	UECRCharacterHealthComponent* HealthComponent;

	UPROPERTY(ReplicatedUsing = OnRep_PawnData, EditAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
	const UECRPawnData* PawnData;

private:
	UFUNCTION()
	void OnRep_PawnData();
};
