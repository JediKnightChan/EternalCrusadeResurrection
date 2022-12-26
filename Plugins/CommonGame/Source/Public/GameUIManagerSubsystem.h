// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/GameInstance.h"

#include "GameUIManagerSubsystem.generated.h"

class ULocalPlayer;
class UGameUIPolicy;
class UCommonLocalPlayer;

/**
 * This manager is intended to be replaced by whatever your game needs to
 * actually create, so this class is abstract to prevent it from being created.
 * 
 * If you just need the basic functionality you will start by sublcassing this
 * subsystem in your own game.
 */
UCLASS(Abstract, config = Game)
class COMMONGAME_API UGameUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	UGameUIManagerSubsystem() { }
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	
	const UGameUIPolicy* GetCurrentUIPolicy() const { return CurrentPolicy; }
	UGameUIPolicy* GetCurrentUIPolicy() { return CurrentPolicy; }

	virtual void NotifyPlayerAdded(UCommonLocalPlayer* LocalPlayer);
	virtual void NotifyPlayerRemoved(UCommonLocalPlayer* LocalPlayer);
	virtual void NotifyPlayerDestroyed(UCommonLocalPlayer* LocalPlayer);

protected:
	void SwitchToPolicy(UGameUIPolicy* InPolicy);

private:
	UPROPERTY(Transient)
	UGameUIPolicy* CurrentPolicy = nullptr;

	UPROPERTY(config, EditAnywhere)
	TSoftClassPtr<UGameUIPolicy> DefaultUIPolicyClass;
};
