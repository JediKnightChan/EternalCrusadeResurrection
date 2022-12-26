// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRInputComponent.h"
#include "System/ECRLocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Settings/ECRSettingsLocal.h"
#include "PlayerMappableInputConfig.h"

UECRInputComponent::UECRInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UECRInputComponent::AddInputMappings(const UECRInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	UECRLocalPlayer* LocalPlayer = InputSubsystem->GetLocalPlayer<UECRLocalPlayer>();
	check(LocalPlayer);

	// Add any registered input mappings from the settings!
	if (UECRSettingsLocal* LocalSettings = UECRSettingsLocal::Get())
	{
		// We don't want to ignore keys that were "Down" when we add the mapping context
		// This allows you to die holding a movement key, keep holding while waiting for respawn,
		// and have it be applied after you respawn immediately. Leaving bIgnoreAllPressedKeysUntilRelease
		// to it's default "true" state would require the player to release the movement key,
		// and press it again when they respawn
		FModifyContextOptions Options = {};
		Options.bIgnoreAllPressedKeysUntilRelease = false;
		
		// Add all registered configs, which will add every input mapping context that is in it
		const TArray<FLoadedMappableConfigPair>& Configs = LocalSettings->GetAllRegisteredInputConfigs();
		for (const FLoadedMappableConfigPair& Pair : Configs)
		{
			if (Pair.bIsActive)
			{
				InputSubsystem->AddPlayerMappableConfig(Pair.Config, Options);	
			}
		}
		
		// Tell enhanced input about any custom keymappings that we have set
		for (const TPair<FName, FKey>& Pair : LocalSettings->GetCustomPlayerInputConfig())
		{
			if (Pair.Key != NAME_None && Pair.Value.IsValid())
			{
				InputSubsystem->AddPlayerMappedKey(Pair.Key, Pair.Value);
			}
		}
	}
}

void UECRInputComponent::RemoveInputMappings(const UECRInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	UECRLocalPlayer* LocalPlayer = InputSubsystem->GetLocalPlayer<UECRLocalPlayer>();
	check(LocalPlayer);
	
	if (UECRSettingsLocal* LocalSettings = UECRSettingsLocal::Get())
	{
		// Remove any registered input contexts
		const TArray<FLoadedMappableConfigPair>& Configs = LocalSettings->GetAllRegisteredInputConfigs();
		for (const FLoadedMappableConfigPair& Pair : Configs)
		{
			InputSubsystem->RemovePlayerMappableConfig(Pair.Config);
		}
		
		// Clear any player mapped keys from enhanced input
		for (const TPair<FName, FKey>& Pair : LocalSettings->GetCustomPlayerInputConfig())
		{
			InputSubsystem->RemovePlayerMappedKey(Pair.Key);
		}
	}
}

void UECRInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}

void UECRInputComponent::AddInputConfig(const FLoadedMappableConfigPair& ConfigPair, UEnhancedInputLocalPlayerSubsystem* InputSubsystem)
{
	check(InputSubsystem);
	if (ensure(ConfigPair.bIsActive))
	{
		InputSubsystem->AddPlayerMappableConfig(ConfigPair.Config);	
	}
}

void UECRInputComponent::RemoveInputConfig(const FLoadedMappableConfigPair& ConfigPair, UEnhancedInputLocalPlayerSubsystem* InputSubsystem)
{
	check(InputSubsystem);
	if (!ConfigPair.bIsActive)
	{
		InputSubsystem->AddPlayerMappableConfig(ConfigPair.Config);	
	}	
}