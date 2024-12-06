#include "WebSocketBase.h"

UWebSocketBase::UWebSocketBase()
{
}

void UWebSocketBase::Connect()
{
	if (!Socket)
	{
		return;
	}
	Socket->Connect();
}

void UWebSocketBase::SendWebSocketMessage(FString MessageString)
{
	if (!Socket)
	{
		return;
	}

	if (!Socket->IsConnected())
	{
		// Don't send if we're not connected.
		return;
	}

	const FString StringMessage = TEXT("Hello there !");
	Socket->Send(StringMessage);
}

void UWebSocketBase::CloseSocket()
{
	if (!Socket)
	{
		return;
	}

	if (!Socket->IsConnected())
	{
		// Don't send if we're not connected.
		return;
	}
	Socket->Close();
}

void UWebSocketBase::Activate(FString NewProtocol, FString NewServerUrl)
{
	if (Socket)
	{
		return;
	}

	Socket = FWebSocketsModule::Get().CreateWebSocket(NewServerUrl, NewProtocol);

	// We bind all available events
	Socket->OnConnected().AddLambda([this]() -> void
	{
		this->OnConnected.Broadcast();
	});

	Socket->OnConnectionError().AddLambda([this](const FString& Error) -> void
	{
		this->OnConnectionError.Broadcast(Error);
	});

	Socket->OnClosed().AddLambda([this](int32 StatusCode, const FString& Reason, bool bWasClean) -> void
	{
		this->OnClosed.Broadcast(StatusCode, Reason, bWasClean);
	});

	Socket->OnMessage().AddLambda([this](const FString& Message) -> void
	{
		this->OnMessage.Broadcast(Message);
	});

	// Socket->OnRawMessage().AddLambda([](const void* Data, SIZE_T Size, SIZE_T BytesRemaining) -> void
	// {
	// 	// This code will run when we receive a raw (binary) message from the server.
	// });

	Socket->OnMessageSent().AddLambda([this](const FString& MessageString) -> void
	{
		this->OnMessageSent.Broadcast(MessageString);
	});
}
