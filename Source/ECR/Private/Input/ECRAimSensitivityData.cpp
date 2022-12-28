// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ECRAimSensitivityData.h"

UECRAimSensitivityData::UECRAimSensitivityData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SensitivityMap =
	{
		{ EECRGamepadSensitivity::Slow,			0.5f },
		{ EECRGamepadSensitivity::SlowPlus,		0.75f },
		{ EECRGamepadSensitivity::SlowPlusPlus,	0.9f },
		{ EECRGamepadSensitivity::Normal,		1.0f },
		{ EECRGamepadSensitivity::NormalPlus,	1.1f },
		{ EECRGamepadSensitivity::NormalPlusPlus,1.25f },
		{ EECRGamepadSensitivity::Fast,			1.5f },
		{ EECRGamepadSensitivity::FastPlus,		1.75f },
		{ EECRGamepadSensitivity::FastPlusPlus,	2.0f },
		{ EECRGamepadSensitivity::Insane,		2.5f },
	};
}

const float UECRAimSensitivityData::SensitivityEnumToFloat(const EECRGamepadSensitivity InSensitivity) const
{
	if (const float* Sens = SensitivityMap.Find(InSensitivity))
	{
		return *Sens;
	}

	return 1.0f;
}
