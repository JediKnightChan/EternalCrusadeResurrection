// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRReplicationGraphSettings.h"
#include "Misc/App.h"
#include "System/ECRReplicationGraph.h"

UECRReplicationGraphSettings::UECRReplicationGraphSettings()
{
	CategoryName = TEXT("Game");
	DefaultReplicationGraphClass = UECRReplicationGraph::StaticClass();
}