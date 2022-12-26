// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "ModularCharacter.generated.h"

/** Minimal class that supports extension by game feature plugins */
UCLASS(Blueprintable)
class MODULARGAMEPLAYACTORS_API AModularCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	//~ Begin AActor Interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface
};
