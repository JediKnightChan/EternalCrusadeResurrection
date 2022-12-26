// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Player/ECRPlayerState.h"
#include "System/ECRLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Gameplay/Character/ECRPawnData.h"
#include "Components/GameFrameworkComponentManager.h"


const FName AECRPlayerState::NAME_ECRAbilityReady("ECRAbilitiesReady");


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


void AECRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
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

void AECRPlayerState::SetPawnData(const UECRPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogECR, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;

	for (const UECRAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_ECRAbilityReady);
	
	ForceNetUpdate();
}

void AECRPlayerState::OnRep_PawnData()
{
}
