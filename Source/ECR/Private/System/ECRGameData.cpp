// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ECRGameData.h"
#include "System/ECRAssetManager.h"

UECRGameData::UECRGameData()
{
}

const UECRGameData& UECRGameData::UECRGameData::Get()
{
	return UECRAssetManager::Get().GetGameData();
}