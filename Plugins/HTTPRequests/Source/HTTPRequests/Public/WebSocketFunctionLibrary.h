// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "WebSocketBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WebSocketFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class HTTPREQUESTS_API UWebSocketFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/** Create web socket */
	UFUNCTION(BlueprintCallable)
	static UWebSocketBase* CreateWebSocket(FString Protocol, FString Url);
};
