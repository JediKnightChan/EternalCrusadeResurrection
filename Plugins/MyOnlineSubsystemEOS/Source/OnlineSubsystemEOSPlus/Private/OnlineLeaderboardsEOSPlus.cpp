// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineLeaderboardsEOSPlus.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemEOSPlus.h"
#include "EOSSettings.h"

FOnlineLeaderboardsEOSPlus::FOnlineLeaderboardsEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	BaseLeaderboardsInterface = EOSPlus->BaseOSS->GetLeaderboardsInterface();
}

FOnlineLeaderboardsEOSPlus::~FOnlineLeaderboardsEOSPlus()
{
	if (BaseLeaderboardsInterface.IsValid())
	{
		BaseLeaderboardsInterface->ClearOnLeaderboardReadCompleteDelegates(this);
		BaseLeaderboardsInterface->ClearOnLeaderboardFlushCompleteDelegates(this);
	}
}

FUniqueNetIdEOSPlusPtr FOnlineLeaderboardsEOSPlus::GetNetIdPlus(const FString& SourceId)
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineLeaderboardsEOSPlus::Initialize()
{
	if (BaseLeaderboardsInterface.IsValid())
	{
		BaseLeaderboardsInterface->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateThreadSafeSP(this, &FOnlineLeaderboardsEOSPlus::OnLeaderboardReadComplete));
		BaseLeaderboardsInterface->AddOnLeaderboardFlushCompleteDelegate_Handle(FOnLeaderboardFlushCompleteDelegate::CreateThreadSafeSP(this, &FOnlineLeaderboardsEOSPlus::OnLeaderboardFlushComplete));
	}
}

// IOnlineLeaderboards Interface

bool FOnlineLeaderboardsEOSPlus::ReadLeaderboards(const TArray< FUniqueNetIdRef >& Players, FOnlineLeaderboardReadRef& ReadObject)
{
	if (BaseLeaderboardsInterface.IsValid())
	{
		bool bArePlayerIdsValid = true;
		TArray< FUniqueNetIdRef > BasePlayerIds;
		for (const FUniqueNetIdRef& Player : Players)
		{
			FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(Player->ToString());
			if (NetIdPlus.IsValid())
			{
				const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
				if (bIsBaseNetIdValid)
				{
					BasePlayerIds.Add(NetIdPlus->GetBaseNetId().ToSharedRef());
				}
				else
				{
					UE_LOG_ONLINE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::ReadLeaderboards] Unable to call method in base interface. Base id not valid for user (%s)."), *Player->ToDebugString());
					bArePlayerIdsValid = false;
					break;
				}
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::ReadLeaderboards] Unable to call method in base interface. User not found (%s)."), *Player->ToDebugString());
				bArePlayerIdsValid = false;
				break;
			}
		}

		if (bArePlayerIdsValid)
		{
			return BaseLeaderboardsInterface->ReadLeaderboards(BasePlayerIds, ReadObject);
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::ReadLeaderboards] Unable to call method in base interface. Base interface not valid."));
	}

	EOSPlus->ExecuteNextTick([this]() {
		TriggerOnLeaderboardReadCompleteDelegates(false);
	});

	return true;
}

bool FOnlineLeaderboardsEOSPlus::ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject)
{
	bool bResult = true;

	if (BaseLeaderboardsInterface.IsValid())
	{
		bResult = BaseLeaderboardsInterface->ReadLeaderboardsForFriends(LocalUserNum, ReadObject);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::ReadLeaderboardsForFriends] Unable to call method in base interface. Base interface not valid."));

		EOSPlus->ExecuteNextTick([this]() {
			TriggerOnLeaderboardReadCompleteDelegates(false);
		});
	}

	return bResult;
}

bool FOnlineLeaderboardsEOSPlus::ReadLeaderboardsAroundRank(int32 Rank, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	bool bResult = true;

	if (BaseLeaderboardsInterface.IsValid())
	{
		bResult = BaseLeaderboardsInterface->ReadLeaderboardsAroundRank(Rank, Range, ReadObject);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::ReadLeaderboardsAroundRank] Unable to call method in base interface. Base interface not valid."));

		EOSPlus->ExecuteNextTick([this]() {
			TriggerOnLeaderboardReadCompleteDelegates(false);
		});
	}

	return bResult;
}

bool FOnlineLeaderboardsEOSPlus::ReadLeaderboardsAroundUser(FUniqueNetIdRef Player, uint32 Range, FOnlineLeaderboardReadRef& ReadObject)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(Player->ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseLeaderboardsInterfaceValid = BaseLeaderboardsInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseLeaderboardsInterfaceValid)
		{
			return BaseLeaderboardsInterface->ReadLeaderboardsAroundUser(NetIdPlus->GetBaseNetId().ToSharedRef(), Range, ReadObject);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ReadLeaderboardsAroundUser] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseLeaderboardsInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseLeaderboardsInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::ReadLeaderboardsAroundUser] Unable to call method in base interface. Unknown user (%s)"), *Player->ToDebugString());
	}

	EOSPlus->ExecuteNextTick([this]() {
		TriggerOnLeaderboardReadCompleteDelegates(false);
	});

	return true;
}

void FOnlineLeaderboardsEOSPlus::FreeStats(FOnlineLeaderboardRead& ReadObject)
{
	if (BaseLeaderboardsInterface.IsValid())
	{
		BaseLeaderboardsInterface->FreeStats(ReadObject);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::FreeStats] Unable to call method in base interface. Base interface not valid."));
	}
}

bool FOnlineLeaderboardsEOSPlus::WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject)
{
	bool bResult = false;

	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(Player.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseLeaderboardsInterfaceValid = BaseLeaderboardsInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseLeaderboardsInterfaceValid)
		{
			bResult = BaseLeaderboardsInterface->WriteLeaderboards(SessionName, *NetIdPlus->GetBaseNetId(), WriteObject);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::WriteLeaderboards] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseLeaderboardsInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseLeaderboardsInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::WriteLeaderboards] Unable to call method in base interface. Unknown user (%s)"), *Player.ToString());
	}

	return bResult;
}

bool FOnlineLeaderboardsEOSPlus::FlushLeaderboards(const FName& SessionName)
{
	bool bResult = true;

	if (BaseLeaderboardsInterface.IsValid())
	{
		bResult = BaseLeaderboardsInterface->FlushLeaderboards(SessionName);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::FlushLeaderboards] Unable to call method in base interface. Base interface not valid."));

		EOSPlus->ExecuteNextTick([this, SessionName]() {
			TriggerOnLeaderboardFlushCompleteDelegates(SessionName, false);
		});
	}

	return bResult;
}

bool FOnlineLeaderboardsEOSPlus::WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores)
{
	bool bResult = false;

	if (BaseLeaderboardsInterface.IsValid())
	{
		bResult = BaseLeaderboardsInterface->WriteOnlinePlayerRatings(SessionName, LeaderboardId, PlayerScores);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineLeaderboardsEOSPlus::WriteOnlinePlayerRatings] Unable to call method in base interface. Base interface not valid."));
	}

	return bResult;
}

void FOnlineLeaderboardsEOSPlus::OnLeaderboardReadComplete(bool bWasSuccessful)
{
	TriggerOnLeaderboardReadCompleteDelegates(bWasSuccessful);
}

void FOnlineLeaderboardsEOSPlus::OnLeaderboardFlushComplete(const FName SessionName, bool bWasSuccessful)
{
	TriggerOnLeaderboardFlushCompleteDelegates(SessionName, bWasSuccessful);
}

// ~IOnlineLeaderboards Interface