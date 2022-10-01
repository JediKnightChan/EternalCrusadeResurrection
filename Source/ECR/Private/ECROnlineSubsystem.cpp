// Copyleft: All rights reversed


#include "ECROnlineSubsystem.h"

#include "ECRGUIPlayerController.h"
#include "ECRUtilsLibrary.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"


#define SETTING_GAMETYPE FName(TEXT("GAMETYPE"))
#define SEARCH_USER_DISPLAY_NAME FName(TEXT("USERDISPLAYNAME"))


FECRSessionResult::FECRSessionResult(const FBlueprintSessionResult BlueprintSessionIn)
{
	BlueprintSession = BlueprintSessionIn;

	// Retrieving match data into this struct
	BlueprintSessionIn.OnlineResult.Session.SessionSettings.Get(SETTING_MAPNAME, MapName);
	BlueprintSessionIn.OnlineResult.Session.SessionSettings.Get(SETTING_GAMETYPE, MatchType);
	BlueprintSessionIn.OnlineResult.Session.SessionSettings.Get(SETTING_GAMEMODE, MatchMode);
	BlueprintSessionIn.OnlineResult.Session.SessionSettings.Get(SEARCH_USER_DISPLAY_NAME, UserDisplayName);
}


UECROnlineSubsystem::UECROnlineSubsystem()
{
	bIsLoggedIn = false;
	OnlineSubsystem = IOnlineSubsystem::Get();
}


void UECROnlineSubsystem::LoginViaEpic(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "accountportal");
}


void UECROnlineSubsystem::LoginViaDevice(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "persistentauth");
}


void UECROnlineSubsystem::Login(FString PlayerName, FString LoginType)
{
	// Setting player display name
	UserDisplayName = PlayerName;

	// Login
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			FOnlineAccountCredentials OnlineAccountCredentials;
			OnlineAccountCredentials.Id = "";
			OnlineAccountCredentials.Token = "";
			OnlineAccountCredentials.Type = LoginType;

			OnlineIdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UECROnlineSubsystem::OnLoginComplete);
			OnlineIdentityPtr->Login(0, OnlineAccountCredentials);
		}
	}
}


void UECROnlineSubsystem::OnLoginComplete(int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId,
                                          const FString& Error)
{
	bIsLoggedIn = bWasSuccessful;

	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			OnlineIdentityPtr->ClearOnLoginCompleteDelegates(0, this);
		}
	}

	const FString SuccessLogin = bWasSuccessful ? "true" : "false";

	UE_LOG(LogTemp, Warning, TEXT("Login successful: %s %s"), *(SuccessLogin), *(Error));


	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			GUISupervisor->ShowMainMenu(true);
		}
		else
		{
			GUISupervisor->HandleLoginFailed(Error);
		}
	}
}


void UECROnlineSubsystem::CreateMatch(const FName SessionName, const int32 NumPublicConnections,
                                      const FString MatchType,
                                      const FString MatchMode, const FString MapName)
{
	// Check if logged in
	if (!bIsLoggedIn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempt to create match, but not logged in!"));
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			FOnlineSessionSettings SessionSettings;
			SessionSettings.NumPublicConnections = NumPublicConnections;
			SessionSettings.bIsDedicated = false;
			SessionSettings.bIsLANMatch = false;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bAllowJoinInProgress = true;
			SessionSettings.bAllowJoinViaPresence = false;
			SessionSettings.bUsesPresence = true;

			// Custom settings
			SessionSettings.Settings.Add(SETTING_MAPNAME, MapName);
			SessionSettings.Settings.Add(SETTING_GAMETYPE, MatchType);
			SessionSettings.Settings.Add(SETTING_GAMEMODE, MatchMode);
			SessionSettings.Settings.Add(SEARCH_USER_DISPLAY_NAME, UserDisplayName);

			OnlineSessionPtr->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UECROnlineSubsystem::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, SessionName, SessionSettings);
		}
	}
}


void UECROnlineSubsystem::FindMatches(const FString MatchType, const FString MatchMode,
                                      const FString MapName)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			SessionSearchSettings = MakeShareable(new FOnlineSessionSearch{});
			SessionSearchSettings->MaxSearchResults = 10;

			// If specified parameters, do filter
			if (MatchType != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_GAMETYPE, MatchType, EOnlineComparisonOp::Equals);
			if (MatchMode != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_GAMEMODE, MatchMode, EOnlineComparisonOp::Equals);
			if (MapName != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_MAPNAME, MapName, EOnlineComparisonOp::Equals);

			OnlineSessionPtr->OnFindSessionsCompleteDelegates.AddUObject(
				this, &UECROnlineSubsystem::OnFindMatchesComplete);
			OnlineSessionPtr->FindSessions(0, SessionSearchSettings.ToSharedRef());
		}
	}
}


void UECROnlineSubsystem::OnCreateMatchComplete(FName SessionName, const bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
		}
	}


	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			// Display loading screen as loading map
			GUISupervisor->ShowLoadingScreen(LoadingMap);
			// Load map with listen parameter
			GetWorld()->ServerTravel("ThirdPersonMap?listen");
		}
		else
		{
			GUISupervisor->HandleCreateMatchFailed();
		}
	}
}


void UECROnlineSubsystem::OnFindMatchesComplete(const bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnFindSessionsCompleteDelegates(this);
		}
	}

	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			TArray<FECRSessionResult> SessionResults;
			for (const FOnlineSessionSearchResult SessionResult : SessionSearchSettings->SearchResults)
			{
				SessionResults.Add(FECRSessionResult{FBlueprintSessionResult{SessionResult}});
			}
			GUISupervisor->HandleFindMatchesSuccess(SessionResults);
		}
		else
		{
			GUISupervisor->HandleFindMatchesFailed();
		}
	}
}
