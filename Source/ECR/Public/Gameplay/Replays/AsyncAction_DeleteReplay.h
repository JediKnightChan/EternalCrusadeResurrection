// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "NetworkReplayStreaming.h"

#include "AsyncAction_DeleteReplay.generated.h"

class AGameStateBase;
class UGameInstance;
class APlayerController;
class AECRPlayerState;
class INetworkReplayStreamer;
class UECRReplayList;
class UECRReplayListEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeleteReplayQueryAsyncDelegate, bool, bSuccess);

/**
 * Deletes replay
 */
UCLASS()
class UAsyncAction_DeleteReplay : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UAsyncAction_DeleteReplay(const FObjectInitializer& ObjectInitializer);

	// Deletes replay
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_DeleteReplay* DeleteReplay(UECRReplayListEntry* Replay);

	virtual void Activate() override;

public:
	// Called when the replay query completes
	UPROPERTY(BlueprintAssignable)
	FDeleteReplayQueryAsyncDelegate QueryComplete;

	FDeleteFinishedStreamCallback OnDeleteFinishedStreamCompleteDelegate;

private:
	void OnDeleteFinishedStreamComplete(const FDeleteFinishedStreamResult& Result);

private:
	UPROPERTY()
	UECRReplayListEntry* ReplayToDelete;

	TWeakObjectPtr<APlayerController> PlayerController;

	TSharedPtr<INetworkReplayStreamer> ReplayStreamer;
};
