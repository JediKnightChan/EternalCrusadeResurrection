// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for mirroring platform leaderboards to EOS leaderboards
 */
class FOnlineLeaderboardsEOSPlus :
	public IOnlineLeaderboards,
	public TSharedFromThis<FOnlineLeaderboardsEOSPlus, ESPMode::ThreadSafe>
{
public:
	FOnlineLeaderboardsEOSPlus() = delete;
	virtual ~FOnlineLeaderboardsEOSPlus();

	void Initialize();

// IOnlineLeaderboards Interface
	virtual bool ReadLeaderboards(const TArray< FUniqueNetIdRef >& Players, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual bool ReadLeaderboardsAroundUser(FUniqueNetIdRef Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject) override;
	virtual void FreeStats(FOnlineLeaderboardRead& ReadObject) override;
	virtual bool WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject) override;
	virtual bool FlushLeaderboards(const FName& SessionName) override;
	virtual bool WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores) override;
// ~IOnlineLeaderboards Interface

PACKAGE_SCOPE:
	FOnlineLeaderboardsEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	void OnLeaderboardReadComplete(bool bWasSuccessful);
	void OnLeaderboardFlushComplete(const FName SessionName, bool bWasSuccessful);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId);

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;

	// We don't support EOS mirroring yet
	// IOnlineLeaderboardsPtr EOSLeaderboardsInterface;
	IOnlineLeaderboardsPtr BaseLeaderboardsInterface;
};

typedef TSharedPtr<FOnlineLeaderboardsEOSPlus, ESPMode::ThreadSafe> FOnlineLeaderboardsEOSPlusPtr;
