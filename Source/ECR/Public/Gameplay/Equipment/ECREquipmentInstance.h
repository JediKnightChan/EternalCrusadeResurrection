// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/GameplayTagStack.h"
#include "GameFramework/Pawn.h"

#include "ECREquipmentInstance.generated.h"

struct FECREquipmentActorToSpawn;

/**
 * UECREquipmentInstance
 *
 * A piece of equipment spawned and applied to a pawn
 */
UCLASS(BlueprintType, Blueprintable)
class UECREquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UECREquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;

	//~End of UObject interface

	UFUNCTION(BlueprintPure, Category=Equipment)
	UObject* GetInstigator() const { return Instigator; }

	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }
    
    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category=Equipment)
    TMap<FName, double> DescribeEquipmentCharacteristics(bool bForEquipped);

	FORCEINLINE TArray<FName> GetVisibilityChannels() { return VisibilityChannels; }

	virtual void SpawnEquipmentActors(const TArray<FECREquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();

	virtual void OnEquipped();
	virtual void OnUnequipped();

	void SetVisibility(bool bNewVisible);

	FORCEINLINE bool GetIsVisible() const { return bVisible; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

	UFUNCTION()
	void OnRep_bVisible();

private:
	UFUNCTION()
	void OnRep_Instigator();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	UObject* Instigator;

	UPROPERTY(Replicated)
	TArray<AActor*> SpawnedActors;

	/** Array of visibility channels in which this item hides other items (eg LeftHand, RightHand for two hand weapon)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<FName> VisibilityChannels;

	/** Tags of modifiers for this item */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FGameplayTagContainer MyModifiersTags;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"), ReplicatedUsing=OnRep_bVisible)
	bool bVisible = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bEquipped = false;

public:
	// Whether want actors for this EI visible when EI is equipped (eg false for hidden items)
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bVisibleOnEquip = true;
};
