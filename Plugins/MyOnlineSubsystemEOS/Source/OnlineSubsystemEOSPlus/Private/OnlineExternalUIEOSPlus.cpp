// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineExternalUIEOSPlus.h"
#include "OnlineSubsystemEOSPlus.h"
#include "OnlineError.h"

FOnlineExternalUIEOSPlus::FOnlineExternalUIEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	BaseExternalUIInterface = EOSPlus->BaseOSS->GetExternalUIInterface();
}

FOnlineExternalUIEOSPlus::~FOnlineExternalUIEOSPlus()
{
	if (BaseExternalUIInterface.IsValid())
	{
		BaseExternalUIInterface->ClearOnExternalUIChangeDelegates(this);
		BaseExternalUIInterface->ClearOnLoginFlowUIRequiredDelegates(this);
		BaseExternalUIInterface->ClearOnCreateAccountFlowUIRequiredDelegates(this);
	}
}

FUniqueNetIdEOSPlusPtr FOnlineExternalUIEOSPlus::GetNetIdPlus(const FString& SourceId) const
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineExternalUIEOSPlus::Initialize()
{
	if (BaseExternalUIInterface.IsValid())
	{
		BaseExternalUIInterface->AddOnExternalUIChangeDelegate_Handle(FOnExternalUIChangeDelegate::CreateThreadSafeSP(this, &FOnlineExternalUIEOSPlus::OnExternalUIChangeBase));
		BaseExternalUIInterface->AddOnLoginFlowUIRequiredDelegate_Handle(FOnLoginFlowUIRequiredDelegate::CreateThreadSafeSP(this, &FOnlineExternalUIEOSPlus::OnLoginFlowUIRequiredBase));
		BaseExternalUIInterface->AddOnCreateAccountFlowUIRequiredDelegate_Handle(FOnCreateAccountFlowUIRequiredDelegate::CreateThreadSafeSP(this, &FOnlineExternalUIEOSPlus::OnOnCreateAccountFlowUIRequiredBase));
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineExternalUIEOSPlus::Initialize] BaseExternalUIInterface delegates not bound. Base interface not valid"));
	}
}

//~ Begin IOnlineExternalUI Interface

bool FOnlineExternalUIEOSPlus::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowLoginUI(ControllerIndex, bShowOnlineOnly, bShowSkipButton, Delegate);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowLoginUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowAccountCreationUI(const int ControllerIndex, const FOnAccountCreationUIClosedDelegate& Delegate)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowAccountCreationUI(ControllerIndex, Delegate);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowAccountCreationUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowFriendsUI(int32 LocalUserNum)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowFriendsUI(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowFriendsUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowInviteUI(int32 LocalUserNum, FName SessionName)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowInviteUI(LocalUserNum, SessionName);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowInviteUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowAchievementsUI(int32 LocalUserNum)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowAchievementsUI(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowAchievementsUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowLeaderboardUI(const FString& LeaderboardName)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowLeaderboardUI(LeaderboardName);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowLeaderboardUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowWebURL(Url, ShowParams, Delegate);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowWebURL] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::CloseWebURL()
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->CloseWebURL();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::CloseWebURL] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate)
{
	bool bResult = false;

	FUniqueNetIdEOSPlusPtr RequestorNetIdPlus = GetNetIdPlus(Requestor.ToString());
	FUniqueNetIdEOSPlusPtr RequesteeNetIdPlus = GetNetIdPlus(Requestee.ToString());
	if (RequestorNetIdPlus.IsValid() && RequesteeNetIdPlus.IsValid())
	{
		const bool bIsRequestorBaseNetIdValid = ensure(RequestorNetIdPlus->GetBaseNetId().IsValid());
		const bool bIsRequesteeBaseNetIdValid = RequesteeNetIdPlus->GetBaseNetId().IsValid(); // We don't ensure because the requestee might not have a valid id in the current platform
		const bool bIsBaseExternalUIInterfaceValid = BaseExternalUIInterface.IsValid();
		if (bIsRequestorBaseNetIdValid && bIsRequesteeBaseNetIdValid && bIsBaseExternalUIInterfaceValid)
		{
			bResult = BaseExternalUIInterface->ShowProfileUI(*RequestorNetIdPlus->GetBaseNetId(), *RequesteeNetIdPlus->GetBaseNetId(), Delegate);
			return bResult;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ShowProfileUI] Unable to call method in base interface. bIsRequestorBaseNetIdValid=%s bIsRequesteeBaseNetIdValid=%s IsBaseExternalUIInterfaceValid=%s."), *LexToString(bIsRequestorBaseNetIdValid), *LexToString(bIsRequesteeBaseNetIdValid), *LexToString(bIsBaseExternalUIInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ShowProfileUI] Unable to call method in base interface. Unknown user (%s)"), RequestorNetIdPlus.IsValid() ? *Requestee.ToString() : *Requestor.ToString());
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	bool bResult = false;

	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UniqueId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseExternalUIInterfaceValid = BaseExternalUIInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseExternalUIInterfaceValid)
		{
			bResult = BaseExternalUIInterface->ShowAccountUpgradeUI(*NetIdPlus->GetBaseNetId());
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ShowAccountUpgradeUI] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseExternalUIInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseExternalUIInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ShowAccountUpgradeUI] Unable to call method in base interface. Unknown user (%s)"), *UniqueId.ToString());
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowStoreUI(LocalUserNum, ShowParams, Delegate);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowStoreUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

bool FOnlineExternalUIEOSPlus::ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	bool bResult = false;

	if (BaseExternalUIInterface.IsValid())
	{
		bResult = BaseExternalUIInterface->ShowSendMessageUI(LocalUserNum, ShowParams, Delegate);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineExternalUIEOSPlus::ShowSendMessageUI] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

void FOnlineExternalUIEOSPlus::OnExternalUIChangeBase(bool bIsOpening)
{
	TriggerOnExternalUIChangeDelegates(bIsOpening);
}

void FOnlineExternalUIEOSPlus::OnLoginFlowUIRequiredBase(const FString& RequestedURL, const FOnLoginRedirectURL& OnLoginRedirect, const FOnLoginFlowComplete& OnLoginFlowComplete, bool& bOutShouldContinueLogin)
{
	TriggerOnLoginFlowUIRequiredDelegates(RequestedURL, OnLoginRedirect, OnLoginFlowComplete, bOutShouldContinueLogin);
}

void FOnlineExternalUIEOSPlus::OnOnCreateAccountFlowUIRequiredBase(const FString& RequestedURL, const FOnLoginRedirectURL& OnLoginRedirect, const FOnLoginFlowComplete& OnLoginFlowComplete, bool& bOutShouldContinueLogin)
{
	TriggerOnCreateAccountFlowUIRequiredDelegates(RequestedURL, OnLoginRedirect, OnLoginFlowComplete, bOutShouldContinueLogin);
}

//~ End IOnlineExternalUI Interface