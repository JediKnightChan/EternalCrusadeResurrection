// Copyleft: All rights reversed


#include "Online/ECROnlineSubsystem.h"
#include "System/ECRLogChannels.h"
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

void UECROnlineSubsystem::LoginViaDevTool(const FString PlayerName, const FString Address, const FString CredName)
{
	Login(PlayerName, "developer", Address, CredName);
}


void UECROnlineSubsystem::Login(FString PlayerName, FString LoginType, FString Id, FString Token)
{
	// Setting player display name
	UserDisplayName = PlayerName;

	// Login
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			FOnlineAccountCredentials OnlineAccountCredentials;
			OnlineAccountCredentials.Id = Id;
			OnlineAccountCredentials.Token = Token;
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


FString UECROnlineSubsystem::GetMatchFactionString(
	const TArray<FFactionAlliance>& FactionAlliances, const TMap<FName, FText>& FactionNamesToShortTexts)
{
	TArray<FString> Sides;
	for (FFactionAlliance Alliance : FactionAlliances)
	{
		TArray<FString> SideFactions;
		for (FName FactionName : Alliance.FactionNames)
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
                                      const TArray<FFactionAlliance> Alliances,
                                      const TMap<FName, int32> FactionNamesToCapacities,
                                      const TMap<FName, FText> FactionNamesToShortTexts)
{
	// Check if logged in
	if (!bIsLoggedIn)
	{
		UE_LOG(LogECR, Error, TEXT("Attempt to create match, but not logged in!"));
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
			for (auto [FactionNames, Strength] : Alliances)
			{
				for (FName FactionName : FactionNames)
				{
					SessionSettings.Set(FName{SETTING_FACTION_PREFIX.ToString() + FactionName.ToString()}, true,
					                    EOnlineDataAdvertisementType::ViaOnlineService);
				}
			}

			FString FactionsString = GetMatchFactionString(Alliances, FactionNamesToShortTexts);
			SessionSettings.Set(SETTING_FACTIONS, FactionsString, EOnlineDataAdvertisementType::ViaOnlineService);

			SessionSettings.Set(SEARCH_USER_DISPLAY_NAME, UserDisplayName,
			                    EOnlineDataAdvertisementType::ViaOnlineService);

			/** Custom settings end */

			// Saving match creation settings for use in delegate and after map load
			MatchCreationSettings = FECRMatchSettings{
				ModeName, MapName, MapPath, MissionName, Alliances, FactionNamesToCapacities
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


void UECROnlineSubsystem::DestroySession()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->OnDestroySessionCompleteDelegates.AddUObject(
				this, &UECROnlineSubsystem::OnDestroySessionComplete);
			OnlineSessionPtr->DestroySession(DEFAULT_SESSION_NAME);
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
			const FString Address = FString::Printf(TEXT("%s?listen?DisplayName=%s"),
			                                        *(MatchCreationSettings.MapPath), *(UserDisplayName));
			GetWorld()->ServerTravel(Address);
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
						const FString Address = FString::Printf(
							TEXT("%s?DisplayName=%s"), *(ConnectionString), *(UserDisplayName));
						GUISupervisor->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
					}
					else
					{
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

void UECROnlineSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnDestroySessionCompleteDelegates(this);
		}
	}
}


FString UECROnlineSubsystem::GetPlayerNickname()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			if (bIsLoggedIn)
			{
				return OnlineIdentityPtr->GetPlayerNickname(0);
			}
		}
	}
	return "";
}

FString UECROnlineSubsystem::NetIdToString(const FUniqueNetIdRepl NetId)
{
	return NetId.ToString();
}
