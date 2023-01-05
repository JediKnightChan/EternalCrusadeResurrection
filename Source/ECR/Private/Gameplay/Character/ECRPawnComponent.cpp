// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Character/ECRPawnComponent.h"


UECRPawnComponent::UECRPawnComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}
