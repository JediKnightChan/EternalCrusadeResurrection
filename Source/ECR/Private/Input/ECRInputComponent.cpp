// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ECRInputComponent.h"
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
		// Tell enhanced input about any custom keymappings that the player may have customized
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
