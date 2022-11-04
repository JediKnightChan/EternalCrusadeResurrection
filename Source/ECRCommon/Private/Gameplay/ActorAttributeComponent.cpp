// Copyleft: All rights reversed


#include "Gameplay/ActorAttributeComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values for this component's properties
UActorAttributeComponent::UActorAttributeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}

void UActorAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UActorAttributeComponent, CurrentValue);
	DOREPLIFETIME(UActorAttributeComponent, MaximumValue);
}

void UActorAttributeComponent::ResetCurrentValueToMax()
{
	CurrentValue = MaximumValue;
	// Call OnRep on server for processing changes
	OnRep_CurrentValue();
}

void UActorAttributeComponent::SetMaxValue(const float NewMaximumValue)
{
	MaximumValue = NewMaximumValue;
	// Call OnRep on server for processing changes
	OnRep_MaximumValue();
}

void UActorAttributeComponent::ApplyDamage(const float Damage)
{
	CurrentValue = FMath::Max(CurrentValue - Damage, 0.0f);
}


void UActorAttributeComponent::OnRep_CurrentValue() const
{
	// ReSharper disable once CppExpressionWithoutSideEffects
	ProcessPlayerParameterChanged.ExecuteIfBound(CurrentValue, MaximumValue);
}

void UActorAttributeComponent::OnRep_MaximumValue() const
{
	// ReSharper disable once CppExpressionWithoutSideEffects
	ProcessPlayerParameterChanged.ExecuteIfBound(CurrentValue, MaximumValue);
}
