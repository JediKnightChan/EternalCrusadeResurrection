// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineAchievementsEOSPlus.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemEOSPlus.h"
#include "EOSSettings.h"

FOnAchievementsWrittenDelegate Ignored;

FOnlineAchievementsEOSPlus::FOnlineAchievementsEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	EosAchievementsInterface = EOSPlus->EosOSS->GetAchievementsInterface();
	check(EosAchievementsInterface.IsValid());

	BaseAchievementsInterface = EOSPlus->BaseOSS->GetAchievementsInterface();
	if (BaseAchievementsInterface.IsValid())
	{
		BaseAchievementsInterface->AddOnAchievementUnlockedDelegate_Handle(FOnAchievementUnlockedDelegate::CreateRaw(this, &FOnlineAchievementsEOSPlus::OnAchievementUnlocked));
	}
}

FOnlineAchievementsEOSPlus::~FOnlineAchievementsEOSPlus()
{
	if (BaseAchievementsInterface.IsValid())
	{
		BaseAchievementsInterface->ClearOnAchievementUnlockedDelegates(this);
	}
}

void FOnlineAchievementsEOSPlus::OnAchievementUnlocked(const FUniqueNetId& PlayerId, const FString& AchievementId)
{
	TriggerOnAchievementUnlockedDelegates(PlayerId, AchievementId);
}

FUniqueNetIdEOSPlusPtr FOnlineAchievementsEOSPlus::GetNetIdPlus(const FString& SourceId)
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineAchievementsEOSPlus::WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
	if (!NetIdPlus.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineAchievementsEOSPlus::WriteAchievements() failed due to unknown user (%s)"), *PlayerId.ToString());
		return;
	}

	if (NetIdPlus->GetBaseNetId().IsValid() && BaseAchievementsInterface.IsValid())
	{
		BaseAchievementsInterface->WriteAchievements(*NetIdPlus->GetBaseNetId(), WriteObject, Delegate);
	}
	if (UEOSSettings::GetSettings().bMirrorAchievementsToEOS)
	{
		if (NetIdPlus->GetEOSNetId().IsValid())
		{
			// Mirror the achievement data to EOS
			EosAchievementsInterface->WriteAchievements(*NetIdPlus->GetEOSNetId(), WriteObject, Ignored);
		}
	}
}

void FOnlineAchievementsEOSPlus::QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
	if (!NetIdPlus.IsValid() || !NetIdPlus->GetBaseNetId().IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineAchievementsEOSPlus::QueryAchievements() failed due to unknown user (%s)"), *PlayerId.ToString());
		return;
	}

	if (BaseAchievementsInterface.IsValid())
	{
		BaseAchievementsInterface->QueryAchievements(*NetIdPlus->GetBaseNetId(), Delegate);
	}
}

void FOnlineAchievementsEOSPlus::QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
	if (!NetIdPlus.IsValid() || !NetIdPlus->GetBaseNetId().IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineAchievementsEOSPlus::QueryAchievementDescriptions() failed due to unknown user (%s)"), *PlayerId.ToString());
		return;
	}

	if (BaseAchievementsInterface.IsValid())
	{
		BaseAchievementsInterface->QueryAchievementDescriptions(*NetIdPlus->GetBaseNetId(), Delegate);
	}
}

EOnlineCachedResult::Type FOnlineAchievementsEOSPlus::GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
	if (!NetIdPlus.IsValid() || !NetIdPlus->GetBaseNetId().IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineAchievementsEOSPlus::GetCachedAchievement() failed due to unknown user (%s)"), *PlayerId.ToString());
		return EOnlineCachedResult::NotFound;
	}

	EOnlineCachedResult::Type Result = EOnlineCachedResult::NotFound;

	if (BaseAchievementsInterface.IsValid())
	{
		Result = BaseAchievementsInterface->GetCachedAchievement(*NetIdPlus->GetBaseNetId(), AchievementId, OutAchievement);
	}

	return Result;
}

EOnlineCachedResult::Type FOnlineAchievementsEOSPlus::GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
	if (!NetIdPlus.IsValid() || !NetIdPlus->GetBaseNetId().IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("FOnlineAchievementsEOSPlus::GetCachedAchievements() failed due to unknown user (%s)"), *PlayerId.ToString());
		return EOnlineCachedResult::NotFound;
	}

	EOnlineCachedResult::Type Result = EOnlineCachedResult::NotFound;

	if (BaseAchievementsInterface.IsValid())
	{
		Result = BaseAchievementsInterface->GetCachedAchievements(*NetIdPlus->GetBaseNetId(), OutAchievements);
	}

	return Result;
}

EOnlineCachedResult::Type FOnlineAchievementsEOSPlus::GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	EOnlineCachedResult::Type Result = EOnlineCachedResult::NotFound;

	if (BaseAchievementsInterface.IsValid())
	{
		Result = BaseAchievementsInterface->GetCachedAchievementDescription(AchievementId, OutAchievementDesc);
	}

	return Result;
}

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsEOSPlus::ResetAchievements(const FUniqueNetId& PlayerId)
{
	bool bResult = false;

	if (BaseAchievementsInterface.IsValid())
	{
		bResult = BaseAchievementsInterface->ResetAchievements(PlayerId);
	}

	return bResult;
}
#endif
