// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ECRMappableConfigPair.h"
#include "System/ECRAssetManager.h"
#include "Settings/ECRSettingsLocal.h"
#include "ICommonUIModule.h"
#include "CommonUISettings.h"
#include "PlayerMappableInputConfig.h"

bool FMappableConfigPair::CanBeActivated() const
{
	const FGameplayTagContainer& PlatformTraits = ICommonUIModule::GetSettings().GetPlatformTraits();

	// If the current platform does NOT have all the dependent traits, then don't activate it
	if (!DependentPlatformTraits.IsEmpty() && !PlatformTraits.HasAll(DependentPlatformTraits))
	{
		return false;
	}

	// If the platform has any of the excluded traits, then we shouldn't activate this config.
	if (!ExcludedPlatformTraits.IsEmpty() && PlatformTraits.HasAny(ExcludedPlatformTraits))
	{
		return false;
	}

	return true;
}

bool FMappableConfigPair::RegisterPair(const FMappableConfigPair& Pair)
{
	UECRAssetManager& AssetManager = UECRAssetManager::Get();

	if (UECRSettingsLocal* Settings = UECRSettingsLocal::Get())
	{
		// Register the pair with the settings, but do not activate it yet
		if (const UPlayerMappableInputConfig* LoadedConfig = AssetManager.GetAsset(Pair.Config))
		{
			Settings->RegisterInputConfig(Pair.Type, LoadedConfig, false);
			return true;
		}	
	}
	
	return false;
}


void FMappableConfigPair::UnregisterPair(const FMappableConfigPair& Pair)
{
	UECRAssetManager& AssetManager = UECRAssetManager::Get();

	if (UECRSettingsLocal* Settings = UECRSettingsLocal::Get())
	{
		if (const UPlayerMappableInputConfig* LoadedConfig = AssetManager.GetAsset(Pair.Config))
		{
			Settings->UnregisterInputConfig(LoadedConfig);
		}
	}
}
