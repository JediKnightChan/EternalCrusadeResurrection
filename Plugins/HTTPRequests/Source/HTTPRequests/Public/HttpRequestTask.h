#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HttpModule.h"
#include "HttpRequestTask.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRequestSuccess, int32, Result, FString, Content);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRequestFailed);


UCLASS()
class HTTPREQUESTS_API UHttpRequestAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FRequestSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FRequestFailed OnFailed;

	UFUNCTION(BlueprintCallable)
	static UHttpRequestAsyncAction*
	HttpRequestAsyncAction(FString Method, FString Url, TMap<FString, FString> Headers, FString Content);

	void OnPostReceivedResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	virtual void Activate() override;

private:
	FString Method;
	FString URL;
	TMap<FString, FString> Headers;
	FString Content;
};
