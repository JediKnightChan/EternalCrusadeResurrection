#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "WebSocketBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWebSocketConnected);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebSocketConnectionError, const FString &, Error);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWebSocketClosed, int32, StatusCode, const FString&, Reason, bool,
                                               bWasClean);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebSocketOnMessage, const FString &, Message);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWebSocketMessageSent, const FString&, MessageString);


UCLASS()
class HTTPREQUESTS_API UWebSocketBase : public UObject
{
	GENERATED_BODY()

	UWebSocketBase();

public:
	UPROPERTY(BlueprintAssignable)
	FWebSocketConnected OnConnected;

	UPROPERTY(BlueprintAssignable)
	FWebSocketConnectionError OnConnectionError;

	UPROPERTY(BlueprintAssignable)
	FWebSocketClosed OnClosed;

	UPROPERTY(BlueprintAssignable)
	FWebSocketOnMessage OnMessage;

	UPROPERTY(BlueprintAssignable)
	FWebSocketMessageSent OnMessageSent;

	UFUNCTION(BlueprintCallable)
	void Connect();

	UFUNCTION(BlueprintCallable)
	void SendWebSocketMessage(FString MessageString);

	UFUNCTION(BlueprintCallable)
	void CloseSocket();

	void Activate(FString NewProtocol, FString NewServerUrl);

private:
	TSharedPtr<IWebSocket> Socket;
};
