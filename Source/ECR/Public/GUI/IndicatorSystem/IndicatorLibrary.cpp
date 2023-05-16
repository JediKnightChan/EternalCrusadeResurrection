// Copyright Epic Games, Inc. All Rights Reserved.

#include "IndicatorLibrary.h"
#include "ECRIndicatorManagerComponent.h"

UIndicatorLibrary::UIndicatorLibrary()
{
}

UECRIndicatorManagerComponent* UIndicatorLibrary::GetIndicatorManagerComponent(AController* Controller)
{
	return UECRIndicatorManagerComponent::GetComponent(Controller);
}
