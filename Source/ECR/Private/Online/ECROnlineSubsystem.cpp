// Copyleft: All rights reversed


#include "Online/ECROnlineSubsystem.h"

#include "GUI/ECRGUIPlayerController.h"
#include "ECRUtilsLibrary.h"
#include "OnlineSubsystem.h"
#include "Algo/Accumulate.h"
#include "Interfaces/OnlineIdentityInterface.h"


#define SETTING_GAME_MISSION FName(TEXT("GAMEMISSION"))
#define SETTING_FACTION_PREFIX FName(TEXT("FACTION_"))
#define SETTING_FACTIONS FName(TEXT("FACTIONS"))
#define SEARCH_USER_DISPLAY_NAME FName(TEXT("USERDISPLAYNAME"))

#define DEFAULT_SESSION_NAME FName(TEXT("DEFAULT_SESSION_NAME"))


FECRMatchResult::FECRMatchResult()
{
}


FECRMatchResult::FECRMatchResult(const FBlueprintSessionResult BlueprintSessionIn)
{
	BlueprintSession = BlueprintSessionIn;

	// Retrieving match data into this struct
	FString StringBuffer;

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_MAPNAME, StringBuffer);
	Map = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_GAME_MISSION, StringBuffer);
	Mission = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_GAMEMODE, StringBuffer);
	Mode = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_FACTIONS, FactionsString);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SEARCH_USER_DISPLAY_NAME, UserDisplayName);
}


FECRMatchSettings::FECRMatchSettings()
{
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


FString UECROnlineSubsystem::GetMatchFactionString(const TMap<FName, int32>& FactionNamesToSides,
                                                   const TMap<FName, FText>& FactionNamesToShortTexts)
{
	TMap<int32, TArray<FName>> GroupedFactions;
	for (TTuple<FName, int> FactionNameAndSide : FactionNamesToSides)
	{
		GroupedFactions.FindOrAdd(FactionNameAndSide.Value).Add(FactionNameAndSide.Key);
	}
	TArray<FString> Sides;
	for (const TTuple<int, TArray<FName>> SideAndFactions : GroupedFactions)
	{
		TArray<FString> SideFactions;
		TArray<FName> FactionNames = SideAndFactions.Value;
		for (FName FactionName : FactionNames)
		{
			const FText* FactionShortNameText = FactionNamesToShortTexts.Find(FactionName);
			SideFactions.Add(FactionShortNameText->ToString());
		}
		Sides.Add(FString::Join(SideFactions, TEXT(", ")));
	}
	return FString::Join(Sides, TEXT(" vs "));
}


void UECROnlineSubsystem::CreateMatch(const FName ModeName, const FName MapName, const FString MapPath,
                                      const FName MissionName,
                                      const TMap<FName, int32> FactionNamesToSides,
                                      const TMap<FName, int32> FactionNamesToCapacities,
                                      const TMap<FName, FText> FactionNamesToShortTexts)
{
	// Check if logged in
	if (!bIsLoggedIn)
	{
		UE_LOG(LogTemp, Error, TEXT("Attempt to create match, but not logged in!"));
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			FOnlineSessionSettings SessionSettings;

			TArray<int32> FactionCapacities;
			FactionNamesToCapacities.GenerateValueArray(FactionCapacities);

			SessionSettings.NumPublicConnections = Algo::Accumulate(FactionCapacities, 0);
			SessionSettings.bIsDedicated = false;
			SessionSettings.bIsLANMatch = false;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bAllowJoinInProgress = true;
			SessionSettings.bAllowJoinViaPresence = false;
			SessionSettings.bUsesPresence = true;

			/** Custom settings **/
			SessionSettings.Set(SETTING_GAMEMODE, ModeName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
			SessionSettings.Set(SETTING_MAPNAME, MapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
			SessionSettings.Set(SETTING_GAME_MISSION, MissionName.ToString(),
			                    EOnlineDataAdvertisementType::ViaOnlineService);

			// Set boolean flags for filtering by participating factions, set string flag for info about sides
			for (TTuple<FName, int> FactionNameAndSide : FactionNamesToSides)
			{
				FName FactionName = FactionNameAndSide.Key;
				SessionSettings.Set(FName{SETTING_FACTION_PREFIX.ToString() + FactionName.ToString()}, true,
				                    EOnlineDataAdvertisementType::ViaOnlineService);
			}

			FString FactionsString = GetMatchFactionString(FactionNamesToSides, FactionNamesToShortTexts);
			SessionSettings.Set(SETTING_FACTIONS, FactionsString, EOnlineDataAdvertisementType::ViaOnlineService);

			SessionSettings.Set(SEARCH_USER_DISPLAY_NAME, UserDisplayName,
			                    EOnlineDataAdvertisementType::ViaOnlineService);

			/** Custom settings end */

			// Saving match creation settings for use in delegate and after map load
			MatchCreationSettings = FECRMatchSettings{
				ModeName, MapName, MapPath, MissionName, FactionNamesToSides, FactionNamesToCapacities
			};

			OnlineSessionPtr->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UECROnlineSubsystem::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, DEFAULT_SESSION_NAME, SessionSettings);
		}
	}
}


void UECROnlineSubsystem::FindMatches(const FString MatchType, const FString MatchMode, const FString MapName)
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
					SETTING_GAME_MISSION, MatchType, EOnlineComparisonOp::Equals);
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

void UECROnlineSubsystem::JoinMatch(const FBlueprintSessionResult Session)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			// Remove all previous delegates
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
			
			OnlineSessionPtr->OnJoinSessionCompleteDelegates.AddUObject(
				this, &UECROnlineSubsystem::OnJoinSessionComplete);

			OnlineSessionPtr->JoinSession(0, DEFAULT_SESSION_NAME, Session.OnlineResult);
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
			const FString MapPathListen = MatchCreationSettings.MapPath + "?listen";
			GetWorld()->ServerTravel(MapPathListen);
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
			TArray<FECRMatchResult> SessionResults;
			for (const FOnlineSessionSearchResult SessionResult : SessionSearchSettings->SearchResults)
			{
				SessionResults.Add(FECRMatchResult{FBlueprintSessionResult{SessionResult}});
			}
			GUISupervisor->HandleFindMatchesSuccess(SessionResults);
		}
		else
		{
			GUISupervisor->HandleFindMatchesFailed();
		}
	}
}


void UECROnlineSubsystem::OnJoinSessionComplete(const FName SessionName,
                                                const EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnJoinSessionCompleteDelegates(this);
			if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
			{
				FString ConnectionString;
				OnlineSessionPtr->GetResolvedConnectString(SessionName, ConnectionString);

				switch (Result)
				{
				case EOnJoinSessionCompleteResult::Type::Success:
					if (!ConnectionString.IsEmpty())
					{
						UE_LOG(LogTemp, Error, TEXT("Connection string is %s"), *(ConnectionString));
						GUISupervisor->ClientTravel(ConnectionString, ETravelType::TRAVEL_Absolute);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("Connection string for session was empty"));
						GUISupervisor->HandleJoinMatchFailed(false, false);
					}
					break;
				case EOnJoinSessionCompleteResult::Type::SessionIsFull:
					GUISupervisor->HandleJoinMatchFailed(true, false);
					break;
				case EOnJoinSessionCompleteResult::Type::SessionDoesNotExist:
					GUISupervisor->HandleJoinMatchFailed(false, true);
					break;
				default:
					GUISupervisor->HandleJoinMatchFailed(false, false);
					break;
				}
			}
		}
	}
}
