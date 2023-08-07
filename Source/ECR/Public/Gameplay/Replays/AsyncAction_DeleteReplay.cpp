// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncAction_DeleteReplay.h"
#include "ECRReplaySubsystem.h"
#include "GameFramework/PlayerController.h"

UAsyncAction_DeleteReplay::UAsyncAction_DeleteReplay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OnDeleteFinishedStreamCompleteDelegate = FDeleteFinishedStreamCallback::CreateUObject(
		this, &ThisClass::OnDeleteFinishedStreamComplete);
}

UAsyncAction_DeleteReplay* UAsyncAction_DeleteReplay::DeleteReplay(UECRReplayListEntry* Replay)
{
	UAsyncAction_DeleteReplay* Action = nullptr;

	if (Replay)
	{
		Action = NewObject<UAsyncAction_DeleteReplay>();
		Action->ReplayToDelete = Replay;
	}

	return Action;
}

void UAsyncAction_DeleteReplay::Activate()
{
	ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();

	ReplayStreamer->DeleteFinishedStream(ReplayToDelete->StreamInfo.Name,
	                                     OnDeleteFinishedStreamCompleteDelegate);
}

void UAsyncAction_DeleteReplay::OnDeleteFinishedStreamComplete(const FDeleteFinishedStreamResult& Result)
{
	QueryComplete.Broadcast(Result.WasSuccessful());
}
