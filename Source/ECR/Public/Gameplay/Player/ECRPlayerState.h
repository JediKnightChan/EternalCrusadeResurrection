// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerState.h"
#include "AbilitySystemInterface.h"

#include "ECRPlayerState.generated.h"

class UECRAbilitySystemComponent;
class UAbilitySystemComponent;
class UECRPawnData;


/**
 * AECRPlayerState
 *
 *	Base player state class used by this project.
 */
UCLASS(Config = Game)
class ECR_API AECRPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AECRPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "ECR|PlayerState")
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const { return AbilitySystemComponent; }

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	void SetPawnData(const UECRPawnData* InPawnData);

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	//~End of APlayerState interface

	static const FName NAME_ECRAbilityReady;
protected:
	UFUNCTION()
	void OnRep_PawnData();

protected:

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	const UECRPawnData* PawnData;

private:

	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "ECR|PlayerState")
	UECRAbilitySystemComponent* AbilitySystemComponent;

};
