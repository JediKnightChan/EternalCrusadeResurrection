// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncAction_QueryReplays.h"
#include "ECRReplaySubsystem.h"
#include "GameFramework/PlayerController.h"

UAsyncAction_QueryReplays::UAsyncAction_QueryReplays(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_QueryReplays* UAsyncAction_QueryReplays::QueryReplays(APlayerController* InPlayerController)
{
	UAsyncAction_QueryReplays* Action = nullptr;

	if (InPlayerController != nullptr)
	{
		Action = NewObject<UAsyncAction_QueryReplays>();
		Action->PlayerController = InPlayerController;
	}

	return Action;
}

void UAsyncAction_QueryReplays::Activate()
{
	ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();

	ResultList = NewObject<UECRReplayList>();
	if (ReplayStreamer.IsValid())
	{
		FNetworkReplayVersion EnumerateStreamsVersion = FNetworkVersion::GetReplayVersion();

		ReplayStreamer->EnumerateStreams(EnumerateStreamsVersion, INDEX_NONE, FString(), TArray<FString>(), FEnumerateStreamsCallback::CreateUObject(this, &ThisClass::OnEnumerateStreamsComplete));
	}
	else
	{
		QueryComplete.Broadcast(ResultList);
	}
}

void UAsyncAction_QueryReplays::OnEnumerateStreamsComplete(const FEnumerateStreamsResult& Result)
{
	for (const FNetworkReplayStreamInfo& StreamInfo : Result.FoundStreams)
	{
		UECRReplayListEntry* NewReplayEntry = NewObject<UECRReplayListEntry>(ResultList);
		NewReplayEntry->StreamInfo = StreamInfo;
		ResultList->Results.Add(NewReplayEntry);
	}

	// Sort demo names by date
	struct FCompareDateTime
	{
		FORCEINLINE bool operator()(const UECRReplayListEntry& A, const UECRReplayListEntry& B) const
		{
			return A.StreamInfo.Timestamp.GetTicks() > B.StreamInfo.Timestamp.GetTicks();
		}
	};

	Sort(ResultList->Results.GetData(), ResultList->Results.Num(), FCompareDateTime());

	QueryComplete.Broadcast(ResultList);
}