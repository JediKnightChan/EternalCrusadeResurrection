// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for mirroring platform achievements to EOS achievements
 */
class FOnlineAchievementsEOSPlus :
	public IOnlineAchievements
{
public:
	FOnlineAchievementsEOSPlus() = delete;
	virtual ~FOnlineAchievementsEOSPlus();

// IOnlineAchievements Interface
	virtual void WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate = FOnAchievementsWrittenDelegate())  override;
	virtual void QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate())  override;
	virtual void QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate())  override;
	virtual EOnlineCachedResult::Type GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)  override;
	virtual EOnlineCachedResult::Type GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements)  override;
	virtual EOnlineCachedResult::Type GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)  override;
#if !UE_BUILD_SHIPPING
	virtual bool ResetAchievements(const FUniqueNetId& PlayerId) override;
#endif
// ~IOnlineAchievements Interface

PACKAGE_SCOPE:
	FOnlineAchievementsEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	void OnAchievementUnlocked(const FUniqueNetId& PlayerId, const FString& AchievementId);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId);

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;
	/** Since we're going to bind to delegates, we need to hold onto these */
	IOnlineAchievementsPtr BaseAchievementsInterface;
	IOnlineAchievementsPtr EosAchievementsInterface;
};

typedef TSharedPtr<FOnlineAchievementsEOSPlus, ESPMode::ThreadSafe> FOnlineAchievementsEOSPlusPtr;
