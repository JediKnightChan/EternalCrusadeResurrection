// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/**
 * when you modify this, please note that this information can be saved with instances
 * also DefaultEngine.ini [/Script/Engine.CollisionProfile] should match with this list
 **/

// Trace against Actors/Components which provide interactions.
#define ECR_TraceChannel_Interaction					ECC_GameTraceChannel1

// Trace used by weapons, will hit physics assets instead of capsules
#define ECR_TraceChannel_Weapon						ECC_GameTraceChannel2

// Trace used by weapons, will hit pawn capsules instead of physics assets
#define ECR_TraceChannel_Weapon_Capsule				ECC_GameTraceChannel3

// Trace used by weapons, will trace through multiple pawns rather than stopping on the first hit
#define ECR_TraceChannel_Weapon_Multi					ECC_GameTraceChannel4

// Allocated to aim assist by the ShooterCore game feature
// ECC_GameTraceChannel5
