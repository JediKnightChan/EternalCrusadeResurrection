// Copyright Epic Games, Inc. All Rights Reserved.

#include "SocketSubsystemEOSUtils_OnlineSubsystemEOS.h"

#include "OnlineSubsystemEOS.h"
#include "UserManagerEOS.h"
#include "OnlineSessionEOS.h"

FSocketSubsystemEOSUtils_OnlineSubsystemEOS::FSocketSubsystemEOSUtils_OnlineSubsystemEOS(FOnlineSubsystemEOS& InSubsystemEOS)
	: SubsystemEOS(InSubsystemEOS)
{
}

FSocketSubsystemEOSUtils_OnlineSubsystemEOS::~FSocketSubsystemEOSUtils_OnlineSubsystemEOS()
{
}

EOS_ProductUserId FSocketSubsystemEOSUtils_OnlineSubsystemEOS::GetLocalUserId()
{
	EOS_ProductUserId Result = nullptr;

	Result = SubsystemEOS.UserManager->GetLocalProductUserId();

	return Result;
}

FString FSocketSubsystemEOSUtils_OnlineSubsystemEOS::GetSessionId()
{
	FString Result;

	const IOnlineSessionPtr DefaultSessionInt = SubsystemEOS.GetSessionInterface();
	if (DefaultSessionInt.IsValid())
	{
		if (const FNamedOnlineSession* const NamedSession = DefaultSessionInt->GetNamedSession(NAME_GameSession))
		{
			Result = NamedSession->GetSessionIdStr();
		}
	}

	return Result;
}

FName FSocketSubsystemEOSUtils_OnlineSubsystemEOS::GetSubsystemInstanceName()
{
	return SubsystemEOS.GetInstanceName();
}
