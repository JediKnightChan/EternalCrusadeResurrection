// Copyleft: All rights reversed


#include "ECRGameInstance.h"

#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"


UECRGameInstance::UECRGameInstance()
{
}

void UECRGameInstance::Init()
{
	Super::Init();

	OnlineSubsystem = IOnlineSubsystem::Get();
}


void UECRGameInstance::Login()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			FOnlineAccountCredentials OnlineAccountCredentials;
			OnlineAccountCredentials.Id = "";
			OnlineAccountCredentials.Token = "";
			// ReSharper disable once StringLiteralTypo
			OnlineAccountCredentials.Type = "accountportal";

			OnlineIdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UECRGameInstance::OnLoginComplete);
			OnlineIdentityPtr->Login(0, OnlineAccountCredentials);
		}
	}
}


void UECRGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
                                       const FString& Error)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			OnlineIdentityPtr->ClearOnLoginCompleteDelegates(0, this);
		}
	}
}


void UECRGameInstance::CreateMatch(const FName MatchName, const int32 NumPublicConnections, const bool bIsDedicated)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			FOnlineSessionSettings SessionSettings;
			SessionSettings.NumPublicConnections = NumPublicConnections;
			SessionSettings.bIsDedicated = bIsDedicated;
			SessionSettings.bIsLANMatch = false;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bAllowJoinInProgress = true;
			SessionSettings.bAllowJoinViaPresence = false;
			SessionSettings.bUsesPresence = true;

			OnlineSessionPtr->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, MatchName, SessionSettings);
		}
	}
}


void UECRGameInstance::OnCreateMatchComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
		}
	}
}
