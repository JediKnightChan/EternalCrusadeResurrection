// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Attributes/ECRPveCombatSet.h"

#include "GameplayEffectExtension.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "System/Messages/ECRVerbMessage.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Resource_Message, "ECR.Resource.Message");

UECRPveCombatSet::UECRPveCombatSet()
	: Resource(0.0f)
{
}

void UECRPveCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// These attributes are only for owner
	DOREPLIFETIME_CONDITION_NOTIFY(UECRPveCombatSet, Resource, COND_OwnerOnly, REPNOTIFY_Always);
}


void UECRPveCombatSet::OnRep_Resource(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRPveCombatSet, Resource, OldValue);
}

void UECRPveCombatSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UECRPveCombatSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UECRPveCombatSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetResourceAttribute())
	{
		// Do not allow resource drop below 0 or go more than 100
		NewValue = FMath::Clamp(NewValue, 0.0f, 100.0f);
	}
}

void UECRPveCombatSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetResourceAttribute())
	{
		// Send a standardized verb message that other systems can observe
		SendResourceChangedMessage(Data);
	}
}


void UECRPveCombatSet::SendResourceChangedMessage(const FGameplayEffectModCallbackData& ResourceData) const
{
	FECRVerbMessage Message;
	Message.Verb = TAG_ECR_Resource_Message;
	Message.Instigator = ResourceData.EffectSpec.GetEffectContext().GetEffectCauser();
	Message.InstigatorTags = *ResourceData.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	Message.Object1 = ResourceData.EffectSpec.GetEffectContext().GetSourceObject();
	Message.Target = GetOwningActor();
	Message.TargetTags = *ResourceData.EffectSpec.CapturedTargetTags.GetAggregatedTags();
	Message.Magnitude = ResourceData.EvaluatedData.Magnitude;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}
