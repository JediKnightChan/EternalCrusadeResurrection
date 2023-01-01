// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRWeaponDebugSettings.h"
#include "Misc/App.h"

UECRWeaponDebugSettings::UECRWeaponDebugSettings()
{
}

FName UECRWeaponDebugSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}
