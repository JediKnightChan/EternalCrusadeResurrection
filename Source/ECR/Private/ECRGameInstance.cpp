// Copyleft: All rights reversed


#include "ECRGameInstance.h"

#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#define SETTING_GAMETYPE FName(TEXT("GAMETYPE"))
#define SEARCH_USER_DISPLAY_NAME FName(TEXT("USERDISPLAYNAME"))

#define LOCTEXT_NAMESPACE "GameInstanceNamespace"


UECRGameInstance::UECRGameInstance()
{
	bIsLoggedIn = false;
}


void UECRGameInstance::Init()
{
	Super::Init();
	OnlineSubsystem = IOnlineSubsystem::Get();
}


void UECRGameInstance::Shutdown()
{
	Super::Shutdown();
}


AECRGUIPlayerController* UECRGameInstance::GetGUISupervisor() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return Cast<AECRGUIPlayerController>(PlayerController);
}


void UECRGameInstance::LoginViaEpic(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "accountportal");
}


void UECRGameInstance::LoginViaDevice(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "persistentauth");
}


void UECRGameInstance::Login(FString PlayerName, FString LoginType)
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

			OnlineIdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UECRGameInstance::OnLoginComplete);
			OnlineIdentityPtr->Login(0, OnlineAccountCredentials);
		}
	}
}


void UECRGameInstance::OnLoginComplete(int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId,
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


	if (AECRGUIPlayerController* GUISupervisor = GetGUISupervisor())
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


void UECRGameInstance::CreateMatch(const FName SessionName, const int32 NumPublicConnections, const FString MatchType,
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
				this, &UECRGameInstance::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, SessionName, SessionSettings);
		}
	}
}


void UECRGameInstance::FindMatches(const FString MatchType, const FString MatchMode,
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
				this, &UECRGameInstance::OnFindSessionsComplete);
			OnlineSessionPtr->FindSessions(0, SessionSearchSettings.ToSharedRef());
		}
	}
}


void UECRGameInstance::OnCreateMatchComplete(FName SessionName, const bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
		}
	}


	if (AECRGUIPlayerController* GUISupervisor = GetGUISupervisor())
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


void UECRGameInstance::OnFindSessionsComplete(const bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnFindSessionsCompleteDelegates(this);
		}
	}

	if (AECRGUIPlayerController* GUISupervisor = GetGUISupervisor())
	{
		if (bWasSuccessful)
		{
			TArray<FBlueprintSessionResult> SessionResults;
			for (const FOnlineSessionSearchResult SessionResult : SessionSearchSettings->SearchResults)
			{
				SessionResults.Add(FBlueprintSessionResult{SessionResult});
			}
			GUISupervisor->HandleFindMatchesSuccess(SessionResults);
		}
		else
		{
			GUISupervisor->HandleFindMatchesFailed();
		}
	}
}

#undef LOCTEXT_NAMESPACE
