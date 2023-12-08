// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "WheeledVehiclePawn.h"
#include "Gameplay/Interaction/InteractionQuery.h"
#include "Gameplay/Interaction/IInteractableTarget.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "ECRWheeledVehiclePawn.generated.h"

class UECRHealthComponent;
class UECRAbilitySet;
class UECRPawnExtensionComponent;
class UECRCameraComponent;
class UECRPawnData;
class UECRCombatSet;
class UECRHealthSet;
class UECRSimpleVehicleHealthSet;

/**
 * 
 */
UCLASS()
class ECR_API AECRWheeledVehiclePawn : public AWheeledVehiclePawn, public IAbilitySystemInterface,
                                       public IGameplayTagAssetInterface, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AECRWheeledVehiclePawn(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "ECR|Vehicle")
	AECRPlayerController* GetECRPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Vehicle")
	AECRPlayerState* GetECRPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|Vehicle")
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void GrantAbilitySets(TArray<UECRAbilitySet*> AbilitySets) const;

	// Begins the death sequence for the character (disables collision, disables movement, etc...)
	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	// Called when the death sequence for the vehicle has completed
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnDeathFinished"))
	void K2_OnDeathFinished();

	void InitPawnDataAndAbilities();

	// Interactions
	/** Blueprint implementable event to get interaction options (like entering) */
	UFUNCTION(BlueprintImplementableEvent)
	TArray<FInteractionOption> GetInteractionOptions(const FInteractionQuery InteractQuery);
	
	//~IInteractableTarget interface
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
										  FInteractionOptionBuilder& OptionBuilder) override;
	//~End of IInteractableTarget interface
private:
	// The ability system component sub-object used by vehicles.
	UPROPERTY(VisibleAnywhere, Category = "ECR|Vehicle")
	UECRAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Vehicle", Meta = (AllowPrivateAccess = "true"))
	UECRPawnExtensionComponent* PawnExtComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Vehicle", Meta = (AllowPrivateAccess = "true"))
	UECRHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ECR|Vehicle", Meta = (AllowPrivateAccess = "true"))
	UECRCameraComponent* CameraComponent;

	UPROPERTY(ReplicatedUsing = OnRep_PawnData, EditAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess="true", ExposeOnSpawn="true"))
	const UECRPawnData* PawnData;

private:
	UFUNCTION()
	void OnRep_PawnData();
};
