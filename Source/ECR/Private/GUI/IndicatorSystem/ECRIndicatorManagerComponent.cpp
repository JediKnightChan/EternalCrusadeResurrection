// Copyright Epic Games, Inc. All Rights Reserved.

#include "GUI/IndicatorSystem/ECRIndicatorManagerComponent.h"
#include "GUI/IndicatorSystem/IndicatorDescriptor.h"

UECRIndicatorManagerComponent::UECRIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

/*static*/ UECRIndicatorManagerComponent* UECRIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<UECRIndicatorManagerComponent>();
	}

	return nullptr;
}

void UECRIndicatorManagerComponent::AddIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void UECRIndicatorManagerComponent::RemoveIndicator(UIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);
	
		OnIndicatorRemoved(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}