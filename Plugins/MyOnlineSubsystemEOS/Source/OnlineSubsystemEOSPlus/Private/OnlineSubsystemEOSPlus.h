// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemEOSPlusPrivate.h"

#include "OnlineStatsEOSPlus.h"
#include "OnlineAchievementsEOSPlus.h"
#include "OnlineUserEOSPlus.h"
#include "OnlineSessionEOSPlus.h"
#include "OnlineLeaderboardsEOSPlus.h"
#include "OnlineStoreEOSPlus.h"
#include "OnlineExternalUIEOSPlus.h"
#include "OnlineVoiceEOSPlus.h"
#include "OnlineTitleFileEOSPlus.h"
#include "OnlineUserCloudEOSPlus.h"


/**
 * OnlineSubsystemEOSPlus - Wrapper OSS that uses both the main platform and EOS OSS
 * hence EOS Plus another OSS
 */
class FOnlineSubsystemEOSPlus : 
	public FOnlineSubsystemImpl
{
public:
	virtual ~FOnlineSubsystemEOSPlus() = default;

// IOnlineSubsystem
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	virtual IOnlineTournamentPtr GetTournamentInterface() const override;
	virtual bool IsLocalPlayer(const FUniqueNetId& UniqueId) const override;

	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;

	virtual bool Init() override;
	virtual void PreUnload() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
//~IOnlineSubsystem

PACKAGE_SCOPE:
/** Only the factory makes instances */
	FOnlineSubsystemEOSPlus() = delete;
	explicit FOnlineSubsystemEOSPlus(FName InInstanceName)
		: FOnlineSubsystemImpl(EOSPLUS_SUBSYSTEM, InInstanceName)
		, BaseOSS(nullptr)
		, EosOSS(nullptr)
	{}

	/** The platform or store specific OSS being used as the primary OSS */
	IOnlineSubsystem* BaseOSS;
	/** The EOS OSS that is being used in conjunction with the platform one */
	IOnlineSubsystem* EosOSS;

	/** The stats interface that routes calls between the two OSSes */
	FOnlineStatsEOSPlusPtr StatsInterfacePtr;
	FOnlineAchievementsEOSPlusPtr AchievementsInterfacePtr;
	FOnlineUserEOSPlusPtr UserInterfacePtr;
	FOnlineSessionEOSPlusPtr SessionInterfacePtr;
	FOnlineLeaderboardsEOSPlusPtr LeaderboardsInterfacePtr;
	FOnlineStoreEOSPlusPtr StoreInterfacePtr;
	FOnlineExternalUIEOSPlusPtr ExternalUIInterfacePtr;
	FOnlineVoiceEOSPlusPtr VoiceInterfacePtr;
	FOnlineTitleFileEOSPlusPtr TitleFileInterfacePtr;
	FOnlineUserCloudEOSPlusPtr UserCloudInterfacePtr;
};

typedef TSharedPtr<FOnlineSubsystemEOSPlus, ESPMode::ThreadSafe> FOnlineSubsystemEOSPlusPtr;

