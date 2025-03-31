// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineStatsEOSPlus.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemEOSPlus.h"
#include "EOSSettings.h"

FUniqueNetIdEOSPlusPtr FOnlineStatsEOSPlus::GetNetIdPlus(const FString& SourceId)
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineStatsEOSPlus::QueryStats(const FUniqueNetIdRef LocalUserId, const FUniqueNetIdRef StatsUser, const FOnlineStatsQueryUserStatsComplete& Delegate)
{
	IOnlineStatsPtr Stats = EOSPlus->BaseOSS->GetStatsInterface();
	if (Stats.IsValid())
	{
		Stats->QueryStats(LocalUserId, StatsUser, Delegate);
	}
}

void FOnlineStatsEOSPlus::QueryStats(const FUniqueNetIdRef LocalUserId, const TArray<FUniqueNetIdRef>& StatUsers, const TArray<FString>& StatNames, const FOnlineStatsQueryUsersStatsComplete& Delegate)
{
	IOnlineStatsPtr Stats = EOSPlus->BaseOSS->GetStatsInterface();
	if (Stats.IsValid())
	{
		Stats->QueryStats(LocalUserId, StatUsers, StatNames, Delegate);
	}
}

TSharedPtr<const FOnlineStatsUserStats> FOnlineStatsEOSPlus::GetStats(const FUniqueNetIdRef StatsUserId) const
{
	IOnlineStatsPtr Stats = EOSPlus->BaseOSS->GetStatsInterface();
	if (Stats.IsValid())
	{
		return Stats->GetStats(StatsUserId);
	}
	return nullptr;
}

FOnlineStatsUpdateStatsComplete IgnoredStatsComplete;

void FOnlineStatsEOSPlus::UpdateStats(const FUniqueNetIdRef LocalUserId, const TArray<FOnlineStatsUserUpdatedStats>& UpdatedUserStats, const FOnlineStatsUpdateStatsComplete& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(LocalUserId->ToString());
	if (NetIdPlus.IsValid())
	{
		// This one is the one that will fire the delegate upon completion
		IOnlineStatsPtr Stats = EOSPlus->BaseOSS->GetStatsInterface();
		if (Stats.IsValid())
		{
			// We send a processed copy of the updated stats to the base interface, with the corresponding base user ids
			TArray<FOnlineStatsUserUpdatedStats> BaseUpdatedUserStats;

			for (const FOnlineStatsUserUpdatedStats& UserStat : UpdatedUserStats)
			{
				FUniqueNetIdEOSPlusPtr UserStatNetIdPlus = GetNetIdPlus(UserStat.Account->ToString());
				if (UserStatNetIdPlus.IsValid())
				{
					FOnlineStatsUserUpdatedStats& BaseUpdatedStats = BaseUpdatedUserStats.Emplace_GetRef(UserStatNetIdPlus->GetBaseNetId().ToSharedRef());
					BaseUpdatedStats.Stats = UserStat.Stats;
				}
			}

			Stats->UpdateStats(NetIdPlus->GetBaseNetId().ToSharedRef(), BaseUpdatedUserStats, Delegate);
		}

		const FEOSSettings& EOSSettings = UEOSSettings::GetSettings();
		if (EOSSettings.bMirrorStatsToEOS)
		{
			// Also write the data to EOS
			IOnlineStatsPtr EOSStats = EOSPlus->EosOSS->GetStatsInterface();
			if (EOSStats.IsValid())
			{
				// We send a processed copy of the updated stats to the EOS interface, with the corresponding EOS user ids
				TArray<FOnlineStatsUserUpdatedStats> EOSUpdatedUserStats;

				for (const FOnlineStatsUserUpdatedStats& UserStat : UpdatedUserStats)
				{
					FUniqueNetIdEOSPlusPtr UserStatNetIdPlus = GetNetIdPlus(UserStat.Account->ToString());

					if (UserStatNetIdPlus.IsValid())
					{
						FOnlineStatsUserUpdatedStats& EOSUpdatedStats = EOSUpdatedUserStats.Emplace_GetRef(UserStatNetIdPlus->GetEOSNetId().ToSharedRef());
						EOSUpdatedStats.Stats = UserStat.Stats;
					}
				}

				EOSStats->UpdateStats(NetIdPlus->GetEOSNetId().ToSharedRef(), EOSUpdatedUserStats, IgnoredStatsComplete);
			}
		}
	}
}

#if !UE_BUILD_SHIPPING
void FOnlineStatsEOSPlus::ResetStats(const FUniqueNetIdRef StatsUserId)
{
	// Only need to forward to the base since EOS doesn't support this
	IOnlineStatsPtr Stats = EOSPlus->BaseOSS->GetStatsInterface();
	if (Stats.IsValid())
	{
		Stats->ResetStats(StatsUserId);
	}
}
#endif

bool FOnlineStatsEOSPlus::TriggerEvent(const FUniqueNetId& PlayerId, const TCHAR* EventName, const FOnlineEventParms& Parms)
{
	IOnlineEventsPtr Events = EOSPlus->BaseOSS->GetEventsInterface();
	if (Events.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
		if (NetIdPlus.IsValid())
		{
			return Events->TriggerEvent(*NetIdPlus->GetBaseNetId(), EventName, Parms);
		}
	}

	return false;
}

void FOnlineStatsEOSPlus::SetPlayerSessionId(const FUniqueNetId& PlayerId, const FGuid& PlayerSessionId)
{
	IOnlineEventsPtr Events = EOSPlus->BaseOSS->GetEventsInterface();
	if (Events.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
		if (NetIdPlus.IsValid())
		{
			Events->SetPlayerSessionId(*NetIdPlus->GetBaseNetId(), PlayerSessionId);
		}
	}
}
