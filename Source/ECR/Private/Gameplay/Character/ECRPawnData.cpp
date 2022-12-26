// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Character/ECRPawnData.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/ECRAbilitySet.h"

UECRPawnData::UECRPawnData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PawnClass = nullptr;
	InputConfig = nullptr;
	DefaultCameraMode = nullptr;
}
