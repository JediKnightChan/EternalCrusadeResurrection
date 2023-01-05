// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECREquipmentDefinition.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"

UECREquipmentDefinition::UECREquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UECREquipmentInstance::StaticClass();
}
