// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerState.h"
#include "AbilitySystemInterface.h"

#include "ECRPlayerState.generated.h"

class UECRAbilitySystemComponent;
class UAbilitySystemComponent;


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

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	//~End of APlayerState interface

private:

	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "ECR|PlayerState")
	UECRAbilitySystemComponent* AbilitySystemComponent;

};
