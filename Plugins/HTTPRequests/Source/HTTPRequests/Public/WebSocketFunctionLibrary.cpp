#include "WebSocketFunctionLibrary.h"

UWebSocketBase* UWebSocketFunctionLibrary::CreateWebSocket(FString Protocol, FString Url)
{
	UWebSocketBase* WebSocketBase = NewObject<UWebSocketBase>();
	WebSocketBase->Activate(Protocol, Url);
	return WebSocketBase;
}
