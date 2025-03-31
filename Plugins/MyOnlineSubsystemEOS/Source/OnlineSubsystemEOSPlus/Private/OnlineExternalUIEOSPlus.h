// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for encapsulating the platform external ui interface
 */
class FOnlineExternalUIEOSPlus :
	public IOnlineExternalUI,
	public TSharedFromThis<FOnlineExternalUIEOSPlus, ESPMode::ThreadSafe>
{
public:
	FOnlineExternalUIEOSPlus() = delete;
	virtual ~FOnlineExternalUIEOSPlus();

	void Initialize();

	//~ Begin IOnlineExternalUI Interface
	virtual bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate = FOnLoginUIClosedDelegate()) override;
	virtual bool ShowAccountCreationUI(const int ControllerIndex, const FOnAccountCreationUIClosedDelegate& Delegate = FOnAccountCreationUIClosedDelegate()) override;
	virtual bool ShowFriendsUI(int32 LocalUserNum) override;
	virtual bool ShowInviteUI(int32 LocalUserNum, FName SessionName = NAME_GameSession) override;
	virtual bool ShowAchievementsUI(int32 LocalUserNum) override;
	virtual bool ShowLeaderboardUI(const FString& LeaderboardName) override;
	virtual bool ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate = FOnShowWebUrlClosedDelegate()) override;
	virtual bool CloseWebURL() override;
	virtual bool ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate = FOnProfileUIClosedDelegate()) override;
	virtual bool ShowAccountUpgradeUI(const FUniqueNetId& UniqueId) override;
	virtual bool ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate = FOnShowStoreUIClosedDelegate()) override;
	virtual bool ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate = FOnShowSendMessageUIClosedDelegate()) override;
	//~ End IOnlineExternalUI Interface

PACKAGE_SCOPE:
	FOnlineExternalUIEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	void OnExternalUIChangeBase(bool bIsOpening);
	void OnLoginFlowUIRequiredBase(const FString& RequestedURL, const FOnLoginRedirectURL& OnLoginRedirect, const FOnLoginFlowComplete& OnLoginFlowComplete, bool& bOutShouldContinueLogin);
	void OnOnCreateAccountFlowUIRequiredBase(const FString& RequestedURL, const FOnLoginRedirectURL& OnLoginRedirect, const FOnLoginFlowComplete& OnLoginFlowComplete, bool& bOutShouldContinueLogin);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId) const;

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;
	
	// We don't support EOS mirroring yet
	IOnlineExternalUIPtr BaseExternalUIInterface;
};

typedef TSharedPtr<FOnlineExternalUIEOSPlus, ESPMode::ThreadSafe> FOnlineExternalUIEOSPlusPtr;
