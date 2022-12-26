// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "Engine/Engine.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UAsyncAction_ListenForGameplayMessage* UAsyncAction_ListenForGameplayMessage::ListenForGameplayMessages(UObject* WorldContextObject, FGameplayTag Channel, UScriptStruct* PayloadType, EGameplayMessageMatch MatchType)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UAsyncAction_ListenForGameplayMessage* Action = NewObject<UAsyncAction_ListenForGameplayMessage>();
	Action->WorldPtr = World;
	Action->ChannelToRegister = Channel;
	Action->MessageStructType = PayloadType;
	Action->MessageMatchType = MatchType;
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_ListenForGameplayMessage::Activate()
{
	if (UWorld* World = WorldPtr.Get())
	{
		if (UGameplayMessageSubsystem::HasInstance(World))
		{
			UGameplayMessageSubsystem& Router = UGameplayMessageSubsystem::Get(World);

			TWeakObjectPtr<UAsyncAction_ListenForGameplayMessage> WeakThis(this);
			ListenerHandle = Router.RegisterListenerInternal(ChannelToRegister,
				[WeakThis](FGameplayTag Channel, const UScriptStruct* StructType, const void* Payload)
				{
					if (UAsyncAction_ListenForGameplayMessage* StrongThis = WeakThis.Get())
					{
						StrongThis->HandleMessageReceived(Channel, StructType, Payload);
					}
				},
				MessageStructType.Get(),
				MessageMatchType);

			return;
		}
	}

	SetReadyToDestroy();
}

void UAsyncAction_ListenForGameplayMessage::SetReadyToDestroy()
{
	ListenerHandle.Unregister();

	Super::SetReadyToDestroy();
}

bool UAsyncAction_ListenForGameplayMessage::GetPayload(int32& OutPayload)
{
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UAsyncAction_ListenForGameplayMessage::execGetPayload)
{
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* MessagePtr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	P_FINISH;

	bool bSuccess = false;

	// Make sure the type we are trying to get through the blueprint node matches the type of the message payload received.
	if ((StructProp != nullptr) && (StructProp->Struct != nullptr) && (MessagePtr != nullptr) && (StructProp->Struct == P_THIS->MessageStructType.Get()) && (P_THIS->ReceivedMessagePayloadPtr != nullptr))
	{
		StructProp->Struct->CopyScriptStruct(MessagePtr, P_THIS->ReceivedMessagePayloadPtr);
		bSuccess = true;
	}

	*(bool*)RESULT_PARAM = bSuccess;
}

void UAsyncAction_ListenForGameplayMessage::HandleMessageReceived(FGameplayTag Channel, const UScriptStruct* StructType, const void* Payload)
{
	if (!MessageStructType.Get() || (MessageStructType.Get() == StructType))
	{
		ReceivedMessagePayloadPtr = Payload;

		OnMessageReceived.Broadcast(this, Channel);

		ReceivedMessagePayloadPtr = nullptr;
	}

	if (!OnMessageReceived.IsBound())
	{
		// If the BP object that created the async node is destroyed, OnMessageReceived will be unbound after calling the broadcast.
		// In this case we can safely mark this receiver as ready for destruction.
		// Need to support a more proactive mechanism for cleanup FORT-340994
		SetReadyToDestroy();
	}
}
