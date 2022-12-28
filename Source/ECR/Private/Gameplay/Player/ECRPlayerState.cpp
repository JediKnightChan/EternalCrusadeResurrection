// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Player/ECRPlayerState.h"
#include "System/ECRLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"

#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Components/GameFrameworkComponentManager.h"


AECRPlayerState::AECRPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UECRAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CreateDefaultSubobject<UECRHealthSet>(TEXT("HealthSet"));
	CreateDefaultSubobject<UECRCombatSet>(TEXT("CombatSet"));

	// AbilitySystemComponent needs to be updated at a high frequency.
	NetUpdateFrequency = 100.0f;
}


void AECRPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AECRPlayerState::Reset()
{
	Super::Reset();
}

void AECRPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);

	if (UECRPawnExtensionComponent* PawnExtComp = UECRPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		PawnExtComp->CheckPawnReadyToInitialize();
	}
}

UAbilitySystemComponent* AECRPlayerState::GetAbilitySystemComponent() const
{
	return GetECRAbilitySystemComponent();
}

void AECRPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}
