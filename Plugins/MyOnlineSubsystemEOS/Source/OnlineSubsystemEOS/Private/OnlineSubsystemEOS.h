// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IOnlineSubsystemEOS.h"
#include "OnlineSubsystemNames.h"

#include COMPILED_PLATFORM_HEADER(EOSHelpers.h)

DECLARE_STATS_GROUP(TEXT("EOS"), STATGROUP_EOS, STATCAT_Advanced);

#if WITH_EOS_SDK

#include "eos_sdk.h"

class FSocketSubsystemEOS;
class IEOSSDKManager;
using IEOSPlatformHandlePtr = TSharedPtr<class IEOSPlatformHandle, ESPMode::ThreadSafe>;

class IVoiceChatUser;
class FEOSVoiceChatUser;
using IVoiceChatPtr = TSharedPtr<class IVoiceChat, ESPMode::ThreadSafe>;
using FOnlineSubsystemEOSVoiceChatUserWrapperRef = TSharedRef<class FOnlineSubsystemEOSVoiceChatUserWrapper, ESPMode::ThreadSafe>;

class FUserManagerEOS;
typedef TSharedPtr<class FUserManagerEOS, ESPMode::ThreadSafe> FUserManagerEOSPtr;

class FOnlineSessionEOS;
typedef TSharedPtr<class FOnlineSessionEOS, ESPMode::ThreadSafe> FOnlineSessionEOSPtr;

class FOnlineStatsEOS;
typedef TSharedPtr<class FOnlineStatsEOS, ESPMode::ThreadSafe> FOnlineStatsEOSPtr;

class FOnlineLeaderboardsEOS;
typedef TSharedPtr<class FOnlineLeaderboardsEOS, ESPMode::ThreadSafe> FOnlineLeaderboardsEOSPtr;

class FOnlineAchievementsEOS;
typedef TSharedPtr<class FOnlineAchievementsEOS, ESPMode::ThreadSafe> FOnlineAchievementsEOSPtr;

class FOnlineStoreEOS;
typedef TSharedPtr<class FOnlineStoreEOS, ESPMode::ThreadSafe> FOnlineStoreEOSPtr;

class FOnlineTitleFileEOS;
typedef TSharedPtr<class FOnlineTitleFileEOS, ESPMode::ThreadSafe> FOnlineTitleFileEOSPtr;

class FOnlineUserCloudEOS;
typedef TSharedPtr<class FOnlineUserCloudEOS, ESPMode::ThreadSafe> FOnlineUserCloudEOSPtr;

typedef TSharedPtr<FPlatformEOSHelpers, ESPMode::ThreadSafe> FPlatformEOSHelpersPtr;

/**
 *	OnlineSubsystemEOS - Implementation of the online subsystem for EOS services
 */
class ONLINESUBSYSTEMEOS_API FOnlineSubsystemEOS : 
	public IOnlineSubsystemEOS
{
public:
	virtual ~FOnlineSubsystemEOS() = default;

	/** Used to be called before RHIInit() */
	static void ModuleInit();
	static void ModuleShutdown();

	FPlatformEOSHelpersPtr GetEOSHelpers() { return EOSHelpersPtr; };

// IOnlineSubsystemEOS
	virtual IVoiceChatUser* GetVoiceChatUserInterface(const FUniqueNetId& LocalUserId) override;
	virtual IEOSPlatformHandlePtr GetEOSPlatformHandle() const override { return EOSPlatformHandle; };

// IOnlineSubsystem
	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;	
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual FText GetOnlineServiceName() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void ReloadConfigs(const TSet<FString>& ConfigSections) override;

	virtual IOnlineGroupsPtr GetGroupsInterface() const override { return nullptr; }
	virtual IOnlinePartyPtr GetPartyInterface() const override { return nullptr; }
	virtual IOnlineTimePtr GetTimeInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override { return nullptr; }
	virtual IOnlineSharingPtr GetSharingInterface() const override { return nullptr; }
	virtual IOnlineMessagePtr GetMessageInterface() const override { return nullptr; }
	virtual IOnlineChatPtr GetChatInterface() const override { return nullptr; }
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override { return nullptr; }
	virtual IOnlineTournamentPtr GetTournamentInterface() const override { return nullptr; }
//~IOnlineSubsystem

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;

// FTSTickerObjectBase
	virtual bool Tick(float DeltaTime) override;

	/** Only the factory makes instances */
	FOnlineSubsystemEOS() = delete;
	explicit FOnlineSubsystemEOS(FName InInstanceName);

	FString ProductId;

	IEOSSDKManager* EOSSDKManager;

	/** EOS handles */
	IEOSPlatformHandlePtr EOSPlatformHandle;
	EOS_HAuth AuthHandle;
	EOS_HUI UIHandle;
	EOS_HFriends FriendsHandle;
	EOS_HUserInfo UserInfoHandle;
	EOS_HPresence PresenceHandle;
	EOS_HConnect ConnectHandle;
	EOS_HSessions SessionsHandle;
	EOS_HStats StatsHandle;
	EOS_HLeaderboards LeaderboardsHandle;
	EOS_HMetrics MetricsHandle;
	EOS_HAchievements AchievementsHandle;
	EOS_HEcom EcomHandle;
	EOS_HTitleStorage TitleStorageHandle;
	EOS_HPlayerDataStorage PlayerDataStorageHandle;

	/** Manager that handles all user interfaces */
	FUserManagerEOSPtr UserManager;
	/** The session interface object */
	FOnlineSessionEOSPtr SessionInterfacePtr;
	/** Stats interface pointer */
	FOnlineStatsEOSPtr StatsInterfacePtr;
	/** Leaderboards interface pointer */
	FOnlineLeaderboardsEOSPtr LeaderboardsInterfacePtr;
	FOnlineAchievementsEOSPtr AchievementsInterfacePtr;
	/** EGS store interface pointer */
	FOnlineStoreEOSPtr StoreInterfacePtr;
	/** Title File interface pointer */
	FOnlineTitleFileEOSPtr TitleFileInterfacePtr;
	/** User Cloud interface pointer */
	FOnlineUserCloudEOSPtr UserCloudInterfacePtr;

	bool bWasLaunchedByEGS;
	bool bIsDefaultOSS;
	bool bIsPlatformOSS;

	TSharedPtr<FSocketSubsystemEOS, ESPMode::ThreadSafe> SocketSubsystem;

	static FPlatformEOSHelpersPtr EOSHelpersPtr;

	FEOSVoiceChatUser* GetEOSVoiceChatUserInterface(const FUniqueNetId& LocalUserId);
	void ReleaseVoiceChatUserInterface(const FUniqueNetId& LocalUserId);

private:
	bool PlatformCreate();

	IVoiceChatPtr VoiceChatInterface;
	TUniqueNetIdMap<FOnlineSubsystemEOSVoiceChatUserWrapperRef> LocalVoiceChatUsers;
};

#else

class ONLINESUBSYSTEMEOS_API FOnlineSubsystemEOS :
	public FOnlineSubsystemImpl
{
public:
	explicit FOnlineSubsystemEOS(FName InInstanceName) :
		FOnlineSubsystemImpl(EOS_SUBSYSTEM, InInstanceName)
	{
	}

	virtual ~FOnlineSubsystemEOS() = default;

	virtual IOnlineSessionPtr GetSessionInterface() const override { return nullptr; }
	virtual IOnlineFriendsPtr GetFriendsInterface() const override { return nullptr; }
	virtual IOnlineGroupsPtr GetGroupsInterface() const override { return nullptr; }
	virtual IOnlinePartyPtr GetPartyInterface() const override { return nullptr; }
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override { return nullptr; }
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override { return nullptr; }
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override { return nullptr; }
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override { return nullptr; }
	virtual IOnlineVoicePtr GetVoiceInterface() const override { return nullptr; }
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override { return nullptr; }
	virtual IOnlineTimePtr GetTimeInterface() const override { return nullptr; }
	virtual IOnlineIdentityPtr GetIdentityInterface() const override { return nullptr; }
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override { return nullptr; }
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override { return nullptr; }
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override { return nullptr; }
	virtual IOnlineSharingPtr GetSharingInterface() const override { return nullptr; }
	virtual IOnlineUserPtr GetUserInterface() const override { return nullptr; }
	virtual IOnlineMessagePtr GetMessageInterface() const override { return nullptr; }
	virtual IOnlinePresencePtr GetPresenceInterface() const override { return nullptr; }
	virtual IOnlineChatPtr GetChatInterface() const override { return nullptr; }
	virtual IOnlineStatsPtr GetStatsInterface() const override { return nullptr; }
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override { return nullptr; }
	virtual IOnlineTournamentPtr GetTournamentInterface() const override { return nullptr; }
	virtual FText GetOnlineServiceName() const override { return NSLOCTEXT("OnlineSubsystemEOS", "OnlineServiceName", "EOS"); }

	virtual bool Init() override { return false; }
	virtual bool Shutdown() override { return true; }
	virtual FString GetAppId() const override { return TEXT(""); }
};

#endif

typedef TSharedPtr<FOnlineSubsystemEOS, ESPMode::ThreadSafe> FOnlineSubsystemEOSPtr;

