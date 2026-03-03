// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "ECRReplicationGraphTypes.h"
#include "ECRReplicationGraphSettings.generated.h"

/**
 * Default settings for the ECR replication graph
 */
UCLASS(config=Game, MinimalAPI)
class UECRReplicationGraphSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()

public:
	UECRReplicationGraphSettings();

public:

	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	bool bEnableReplicationGraph = true;
	
	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	bool bEnableDynamicSpatializationFrequency = false;

	UPROPERTY(config, EditAnywhere, Category = FastSharedPath)
	bool bEnableFastSharedPath = false;

	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	int32 AssumedTickRate = 30;

	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph, meta = (MetaClass = "/Script/ECR.ECRReplicationGraph"))
	FSoftClassPath DefaultReplicationGraphClass;

	// How much bandwidth to use for FastShared movement updates. This is counted independently of the NetDriver's target bandwidth.
	UPROPERTY(EditAnywhere, Category = FastSharedPath, meta = (ForceUnits=Kilobytes, ConsoleVariable = "ECR.RepGraph.TargetKBytesSecFastSharedPath"))
	int32 TargetKBytesSecFastSharedPath = 200;

	UPROPERTY(EditAnywhere, Category = FastSharedPath, meta = (ConsoleVariable = "ECR.RepGraph.FastSharedPathCullDistPct"))
	float FastSharedPathCullDistPct = 0.80f;

	UPROPERTY(EditAnywhere, Category = DestructionInfo, meta = (ForceUnits = cm, ConsoleVariable = "ECR.RepGraph.DestructInfo.MaxDist"))
	float DestructionInfoMaxDist = 30000.f;

	UPROPERTY(EditAnywhere, Category=SpatialGrid, meta=(ForceUnits=cm, ConsoleVariable = "ECR.RepGraph.CellSize"))
	float SpatialGridCellSize = 10000.0f;

	// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	UPROPERTY(EditAnywhere, Category=SpatialGrid, meta=(ForceUnits=cm, ConsoleVariable = "ECR.RepGraph.SpatialBiasX"))
	float SpatialBiasX = -200000.0f;

	// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
	UPROPERTY(EditAnywhere, Category=SpatialGrid, meta=(ForceUnits=cm, ConsoleVariable = "ECR.RepGraph.SpatialBiasY"))
	float SpatialBiasY = -200000.0f;

	UPROPERTY(EditAnywhere, Category=SpatialGrid, meta = (ConsoleVariable = "ECR.RepGraph.DisableSpatialRebuilds"))
	bool bDisableSpatialRebuilds = true;

	// How many buckets to spread dynamic, spatialized actors across.
	// High number = more buckets = smaller effective replication frequency.
	// This happens before individual actors do their own NetUpdateFrequency check.
	UPROPERTY(EditAnywhere, Category = DynamicSpatialFrequency, meta = (ConsoleVariable = "ECR.RepGraph.DynamicActorFrequencyBuckets"))
	int32 DynamicActorFrequencyBuckets = 3;

	// Array of Custom Settings for Specific Classes 
	UPROPERTY(config, EditAnywhere, Category = ReplicationGraph)
	TArray<FRepGraphActorClassSettings> ClassSettings;
};