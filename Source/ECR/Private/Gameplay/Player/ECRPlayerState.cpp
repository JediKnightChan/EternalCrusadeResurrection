// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Player/ECRPlayerState.h"

#include "OnlineSubsystemTypes.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Character/ECRPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"


AECRPlayerState::AECRPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void AECRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// FDoRepLifetimeParams SharedParams;
	// SharedParams.bIsPushBased = true;

	DOREPLIFETIME(ThisClass, StatTags);
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

void AECRPlayerState::SetTempNetId(const FString SomeString)
{
	check(GetUniqueId().ToString().IsEmpty())
	const FUniqueNetIdStringRef Id = FUniqueNetIdString::Create(SomeString, FName{"PIE_NetId"});
	SetUniqueId(FUniqueNetIdRepl{Id.Get()});
}

void AECRPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AECRPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AECRPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

void AECRPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
