// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRGameData.h"
#include "ECRAssetManager.h"

UECRGameData::UECRGameData()
{
}

const UECRGameData& UECRGameData::UECRGameData::Get()
{
	return UECRAssetManager::Get().GetGameData();
}