#include "HttpRequestTask.h"


#include "Interfaces/IHttpResponse.h"


void UHttpRequestAsyncAction::Activate()
{
	Super::Activate();

	FHttpModule* Module = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnPostReceivedResponse);

	Request->SetURL(URL);
	Request->SetVerb(Method);
	for (auto [HeaderKey, HeaderValue] : Headers)
	{
		Request->SetHeader(HeaderKey, HeaderValue);
	}
	Request->SetContentAsString(Content);
	Request->ProcessRequest();
}

UHttpRequestAsyncAction* UHttpRequestAsyncAction::HttpRequestAsyncAction(
	FString Method, FString Url, TMap<FString, FString> Headers, FString Content)
{
	UHttpRequestAsyncAction* MyAction = NewObject<UHttpRequestAsyncAction>();
	MyAction->Method = Method;
	MyAction->URL = Url;
	MyAction->Headers = Headers;
	MyAction->Content = Content;
	return MyAction;
}

void UHttpRequestAsyncAction::OnPostReceivedResponse(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                     bool bWasSuccessful)
{
	if (bWasSuccessful && Response.Get())
	{
		OnSuccess.Broadcast(Response->GetResponseCode(), Response->GetContentAsString());
	}
	else
	{
		OnFailed.Broadcast();
	}
}
