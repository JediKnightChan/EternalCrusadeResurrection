// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NetworkReplayStreaming.h"

#include "ECRReplaySubsystem.generated.h"

class UDemoNetDriver;

// An available replay
UCLASS(BlueprintType)
class UECRReplayListEntry : public UObject
{
	GENERATED_BODY()

public:
	FNetworkReplayStreamInfo StreamInfo;

	/** The UI friendly name of the stream */
	UFUNCTION(BlueprintPure, Category=Replays)
	FString GetFriendlyName() const { return StreamInfo.FriendlyName; }

	/** The date and time the stream was recorded */
	UFUNCTION(BlueprintPure, Category=Replays)
	FDateTime GetTimestamp() const { return StreamInfo.Timestamp; }

	/** The duration of the stream in MS */
	UFUNCTION(BlueprintPure, Category=Replays)
	FTimespan GetDuration() const { return FTimespan::FromMilliseconds(StreamInfo.LengthInMS); }

	/** Number of viewers viewing this stream */
	UFUNCTION(BlueprintPure, Category=Replays)
	int32 GetNumViewers() const { return StreamInfo.NumViewers; }

	/** True if the stream is live and the game hasn't completed yet */
	UFUNCTION(BlueprintPure, Category=Replays)
	bool GetIsLive() const { return StreamInfo.bIsLive; }
};

// Results of querying for replays
UCLASS(BlueprintType)
class UECRReplayList : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category=Replays)
	TArray<UECRReplayListEntry*> Results;
};

UCLASS()
class UECRReplaySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UECRReplaySubsystem();

	UFUNCTION(BlueprintCallable, Category=Replays)
	void PlayReplay(UECRReplayListEntry* Replay);

	UFUNCTION(BlueprintCallable, Category=Replays)
	void StartRecordingReplay(FString Name, FString FriendlyName);

	UFUNCTION(BlueprintCallable, Category=Replays)
	void StopRecordingReplay();

	UFUNCTION(BlueprintCallable, Category=Replays)
	void PausePlayingReplay();

	UFUNCTION(BlueprintCallable, Category=Replays)
	void ResumePlayingReplay();

	UFUNCTION(BlueprintCallable, Category=Replays)
	void ChangeReplayPlayingSpeed(float NewSpeed);

	//void DeleteReplay();

	UFUNCTION(BlueprintCallable, Category=Replays)
	void SeekInActiveReplay(float TimeInSeconds);

	UFUNCTION(BlueprintCallable, Category = Replays, BlueprintPure = false)
	float GetReplayLengthInSeconds() const;

	UFUNCTION(BlueprintCallable, Category=Replays, BlueprintPure=false)
	float GetReplayCurrentTime() const;

private:
	UDemoNetDriver* GetDemoDriver() const;
};
