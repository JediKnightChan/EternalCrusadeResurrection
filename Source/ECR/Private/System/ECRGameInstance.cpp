// Copyleft: All rights reversed


#include "System/ECRGameInstance.h"
#include "System/ECRLogChannels.h"
#include "System/MatchSettings.h"
#include "GUI/ECRGUIPlayerController.h"
#include "ECRUtilsLibrary.h"
#include "OnlineSubsystem.h"
#include "Algo/Accumulate.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"


UECRGameInstance::UECRGameInstance()
{
	bDeprecatedIsLoggedIn = false;
	OnlineSubsystem = IOnlineSubsystem::Get();
}

void UECRGameInstance::LogOut()
{
	// Login
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			DestroyParty();
			OnlineIdentityPtr->OnLogoutCompleteDelegates->AddUObject(this, &UECRGameInstance::OnLogoutComplete);
			OnlineIdentityPtr->Logout(0);
		}
	}
}


void UECRGameInstance::LoginViaEpic(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "accountportal");
}


void UECRGameInstance::LoginPersistent(const FString PlayerName)
{
	// ReSharper disable once StringLiteralTypo
	Login(PlayerName, "persistentauth", "", "");
}

void UECRGameInstance::LoginViaDevTool(const FString PlayerName, const FString Address, const FString CredName)
{
	Login(PlayerName, "developer", Address, CredName);
}


void UECRGameInstance::Login(FString PlayerName, FString LoginType, FString Id, FString Token)
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

			OnlineIdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UECRGameInstance::OnLoginComplete);
			OnlineIdentityPtr->Login(0, OnlineAccountCredentials);
		}
	}
}


void UECRGameInstance::OnLoginComplete(int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId,
                                       const FString& Error)
{
	bDeprecatedIsLoggedIn = bWasSuccessful;

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
			// GUISupervisor->ShowMainMenu(true);
			StartListeningForPartyEvents();
			TMap<FString, FString> TestMap;
			TestMap.Add("Test", "Test");
			CreateParty(TestMap);
		}
		else
		{
			GUISupervisor->HandleLoginFailed(Error);
		}
	}
}

void UECRGameInstance::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			OnlineIdentityPtr->ClearOnLogoutCompleteDelegates(0, this);
		}
	}

	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			bDeprecatedIsLoggedIn = false;
			GUISupervisor->HandleLogoutSuccess();
		}
		else
		{
			GUISupervisor->HandleLogoutFailure();
		}
	}
}


FString UECRGameInstance::GetMatchFactionString(
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


void UECRGameInstance::CreateMatch(const FString GameVersion, const FString InGameUniqueIdForSearch,
                                   const FName ModeName, const FName MapName,
                                   const FString MapPath, const FName MissionName,
                                   const FName RegionName, const double TimeDelta,
                                   const FName WeatherName, const FName DayTimeName,
                                   const TArray<FFactionAlliance> Alliances,
                                   const TMap<FName, int32> FactionNamesToCapacities,
                                   const TMap<FName, FText> FactionNamesToShortTexts)
{
	if (IsDedicatedServerInstance())
	{
		UserDisplayName = "SERVER";
	}
	else
	{
		// Check if logged in
		if (!GetIsLoggedIn())
		{
			UE_LOG(LogECR, Error, TEXT("Attempt to create match, but not logged in!"));
			return;
		}
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			// Saving match creation settings for use in delegate and after map load
			MatchCreationSettings = FECRMatchSettings{
				GameVersion, InGameUniqueIdForSearch, ModeName, MapName, MapPath, MissionName, RegionName, WeatherName,
				DayTimeName, TimeDelta,
				Alliances, FactionNamesToCapacities, FactionNamesToShortTexts
			};
			FOnlineSessionSettings SessionSettings = GetSessionSettings();

			// Remove all previous delegates
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
			OnlineSessionPtr->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, DEFAULT_SESSION_NAME, SessionSettings);
		}
	}
}


void UECRGameInstance::FindMatches(const FString GameVersion, const FString MatchType, const FString MatchMode,
                                   const FString MapName, const FString
                                   RegionName)
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
			if (RegionName != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_REGION, RegionName, EOnlineComparisonOp::Equals);
			if (GameVersion != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_GAME_VERSION, GameVersion, EOnlineComparisonOp::Equals);

			OnlineSessionPtr->ClearOnFindSessionsCompleteDelegates(this);
			OnlineSessionPtr->OnFindSessionsCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnFindMatchesComplete);
			OnlineSessionPtr->FindSessions(0, SessionSearchSettings.ToSharedRef());
		}
	}
}

void UECRGameInstance::FindMatchByUniqueInGameId(const FString GameVersion, const FString MatchId)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			SessionSearchSettings = MakeShareable(new FOnlineSessionSearch{});
			SessionSearchSettings->MaxSearchResults = 1;

			// If specified parameters, do filter
			if (GameVersion != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_GAME_VERSION, GameVersion, EOnlineComparisonOp::Equals);

			if (MatchId != "")
				SessionSearchSettings->QuerySettings.Set(
					SETTING_IN_GAME_UNIQUE_ID_FOR_SEARCH, MatchId, EOnlineComparisonOp::Equals);

			OnlineSessionPtr->ClearOnFindSessionsCompleteDelegates(this);
			OnlineSessionPtr->OnFindSessionsCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnFindMatchByUniqueIdComplete);
			OnlineSessionPtr->FindSessions(0, SessionSearchSettings.ToSharedRef());
		}
	}
}


void UECRGameInstance::JoinMatch(const FBlueprintSessionResult Session)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			// Remove all previous delegates
			OnlineSessionPtr->ClearOnJoinSessionCompleteDelegates(this);

			OnlineSessionPtr->OnJoinSessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnJoinSessionComplete);

			OnlineSessionPtr->JoinSession(0, DEFAULT_SESSION_NAME, Session.OnlineResult);
		}
	}
}


void UECRGameInstance::UpdateSessionSettings()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			FOnlineSessionSettings SessionSettings = GetSessionSettings();
			OnlineSessionPtr->UpdateSession(DEFAULT_SESSION_NAME, SessionSettings, true);
		}
	}
}

void UECRGameInstance::UpdateSessionCurrentPlayerAmount(const int32 NewPlayerAmount)
{
	MatchCreationSettings.CurrentPlayerAmount = NewPlayerAmount;
	UpdateSessionSettings();
}

void UECRGameInstance::UpdateSessionMatchStartedTimestamp(const double NewTimestamp)
{
	MatchCreationSettings.MatchStartedTime = NewTimestamp;
	UpdateSessionSettings();
}

void UECRGameInstance::UpdateSessionDayTime(const FName NewDayTime)
{
	MatchCreationSettings.DayTimeName = NewDayTime;
	UpdateSessionSettings();
}


void UECRGameInstance::DestroySession()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->OnDestroySessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnDestroySessionComplete);
			OnlineSessionPtr->DestroySession(DEFAULT_SESSION_NAME);
		}
	}
}


void UECRGameInstance::OnCreateMatchComplete(FName SessionName, const bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("Creating match complete"))
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


void UECRGameInstance::OnFindMatchesComplete(const bool bWasSuccessful)
{
	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			TArray<FECRMatchResult> SessionResults;
			for (const FOnlineSessionSearchResult& SessionResult : SessionSearchSettings->SearchResults)
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

void UECRGameInstance::OnFindMatchByUniqueIdComplete(bool bWasSuccessful)
{
	if (AECRGUIPlayerController* GUISupervisor = UECRUtilsLibrary::GetGUISupervisor(GetWorld()))
	{
		if (bWasSuccessful)
		{
			TArray<FECRMatchResult> SessionResults;
			for (const FOnlineSessionSearchResult& SessionResult : SessionSearchSettings->SearchResults)
			{
				SessionResults.Add(FECRMatchResult{FBlueprintSessionResult{SessionResult}});
			}
			GUISupervisor->HandleFindUniqueMatchByIdOutcome(SessionResults, true);
		}
		else
		{
			GUISupervisor->HandleFindUniqueMatchByIdOutcome({}, false);
		}
	}
}


void UECRGameInstance::OnJoinSessionComplete(const FName SessionName,
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

void UECRGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnDestroySessionCompleteDelegates(this);
		}
	}
}

void UECRGameInstance::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
                                                 const FString& ErrorStr)
{
	TArray<FECRFriendData> Result;
	bool bResultSuccess = false;

	if (bWasSuccessful)
	{
		if (OnlineSubsystem)
		{
			if (IOnlineFriendsPtr Friends = OnlineSubsystem->GetFriendsInterface())
			{
				bResultSuccess = true;

				TArray<TSharedRef<FOnlineFriend>> FriendsArr;
				Friends->GetFriendsList(0, TEXT(""), FriendsArr);
				for (const TSharedRef<FOnlineFriend>& Friend : FriendsArr)
				{
					FOnlineUserPresence OnlineUserPresence = Friend->GetPresence();

					FUniqueNetIdRepl SessionId;
					FUniqueNetIdPtr SessionIdPtr = OnlineUserPresence.SessionId;
					if (SessionIdPtr.IsValid())
					{
						SessionId = FUniqueNetIdRepl{SessionIdPtr.ToSharedRef().Get()};
					}
					FECRFriendData Data{
						OnlineUserPresence.bIsPlayingThisGame != 0,
						Friend->GetDisplayName(),
						OnlineUserPresence.bIsJoinable != 0,
						SessionId,
						Friend->GetUserId().Get()
					};

					Result.Add(Data);
				}
			}
		}
	}

	OnFriendListUpdated_BP.Broadcast(bResultSuccess, Result);
}

void UECRGameInstance::OnPartyCreationComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("Creating party complete"))
	OnPartyCreationFinished_BP.Broadcast(bWasSuccessful);

	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				FNamedOnlineSession* NamedOnlineSession = SessionInterface->GetNamedSession(PARTY_LOBBY_SESSION_NAME);
				if (NamedOnlineSession)
				{
					UE_LOG(LogTemp, Warning, TEXT("Party id %s"), *NamedOnlineSession->GetSessionIdStr())
				}
			}
		}
	}
}

void UECRGameInstance::OnPartyInviteAcceptedByMe(const bool bWasSuccessful, const int32 ControllerId,
                                                 FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult)
{
	if (bWasSuccessful)
	{
		if (OnlineSubsystem)
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				// Kick all players from party I host, it should be empty since I joined other one
				if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
				{
					const FUniqueNetIdPtr Me = OnlineIdentityPtr->GetUniquePlayerId(0);
					if (FNamedOnlineSession* HostSession = SessionInterface->GetNamedSession(PARTY_LOBBY_SESSION_NAME))
					{
						TArray<FUniqueNetIdRef> Members = HostSession->RegisteredPlayers;
						for (FUniqueNetIdRef Member : Members)
						{
							if (Member != Me)
							{
								SessionInterface->RemovePlayerFromSession(0, PARTY_LOBBY_SESSION_NAME, Member.Get());
							}
						}
					}
				}

				// Remove all previous delegates
				SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
				SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
					this, &UECRGameInstance::OnJoinPartyComplete);
				SessionInterface->JoinSession(0, PARTY_LOBBY_CLIENT_SESSION_NAME, InviteResult);
			}
		}
	}
}

void UECRGameInstance::OnJoinPartyComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Type::Success;
	OnPartyJoinFinished_BP.Broadcast(bSuccess);
}

void UECRGameInstance::OnPartyLeaveComplete(FName SessionName, bool bSuccess)
{
	OnPartyLeaveFinished_BP.Broadcast(bSuccess);
}

void UECRGameInstance::OnPartyMembersChanged(FName SessionName, const FUniqueNetId& UniqueId, bool bJoined)
{
	OnPartyMembersChanged_BP.Broadcast();
}

void UECRGameInstance::OnPartyMemberDataChanged(FName SessionName, const FUniqueNetId& TargetUniqueNetId,
                                                const FOnlineSessionSettings& SessionSettings)
{
	OnPartyMembersChanged_BP.Broadcast();
}

void UECRGameInstance::OnPartyMemberLeft(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId)
{
	OnPartyMembersChanged_BP.Broadcast();
}

void UECRGameInstance::OnPartyDataReceived(FName SessionName, const FOnlineSessionSettings& NewSettings)
{
	OnPartyDataUpdated_BP.Broadcast(UECROnlineSubsystem::ConvertSessionSettingsToJson(NewSettings));
}

void UECRGameInstance::OnSessionFailure(const FUniqueNetId& PlayerId, ESessionFailure::Type Reason)
{
	OnDisconnectedFromSession_BP.Broadcast();
}

FOnlineSessionSettings UECRGameInstance::GetSessionSettings()
{
	FOnlineSessionSettings SessionSettings;

	TArray<int32> FactionCapacities;
	MatchCreationSettings.FactionNamesToCapacities.GenerateValueArray(FactionCapacities);

	const bool bIsDedicatedServer = IsDedicatedServerInstance();
	SessionSettings.NumPublicConnections = Algo::Accumulate(FactionCapacities, 0);
	SessionSettings.bIsDedicated = bIsDedicatedServer;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = false;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bUseLobbiesIfAvailable = false;

	/** Custom settings **/
	SessionSettings.Set(SETTING_IN_GAME_UNIQUE_ID_FOR_SEARCH, MatchCreationSettings.InGameUniqueIdForSearch,
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_GAMEMODE, MatchCreationSettings.GameMode.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_MAPNAME, MatchCreationSettings.MapName.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_GAME_MISSION, MatchCreationSettings.GameMission.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_USER_DISPLAY_NAME, UserDisplayName,
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_GAME_VERSION, MatchCreationSettings.GameVersion,
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_WEATHER_NAME, MatchCreationSettings.WeatherName.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_DAYTIME_NAME, MatchCreationSettings.DayTimeName.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_REGION, MatchCreationSettings.Region.ToString(),
	                    EOnlineDataAdvertisementType::ViaOnlineService);

	// Set boolean flags for filtering by participating factions, set string flag for info about sides
	for (auto [FactionNames, Strength] : MatchCreationSettings.Alliances)
	{
		for (FName FactionName : FactionNames)
		{
			SessionSettings.Set(FName{SETTING_FACTION_PREFIX.ToString() + FactionName.ToString()}, true,
			                    EOnlineDataAdvertisementType::ViaOnlineService);
		}
	}

	FString FactionsString = GetMatchFactionString(MatchCreationSettings.Alliances,
	                                               MatchCreationSettings.FactionNamesToShortTexts);
	SessionSettings.Set(SETTING_FACTIONS, FactionsString, EOnlineDataAdvertisementType::ViaOnlineService);


	// Real updatable values
	const FString PlayerAmountString = FString::FromInt(MatchCreationSettings.CurrentPlayerAmount);
	SessionSettings.Set(SETTING_CURRENT_PLAYER_AMOUNT, PlayerAmountString,
	                    EOnlineDataAdvertisementType::ViaOnlineService);
	SessionSettings.Set(SETTING_STARTED_TIME, MatchCreationSettings.MatchStartedTime,
	                    EOnlineDataAdvertisementType::ViaOnlineService);

	/** Custom settings end */

	return SessionSettings;
}

FOnlineSessionSettings UECRGameInstance::GetPartySessionSettings()
{
	FOnlineSessionSettings SessionSettings;
	SessionSettings.NumPublicConnections = 4;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = false;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.bAllowInvites = true;
	return SessionSettings;
}


FString UECRGameInstance::GetPlayerNickname()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			return OnlineIdentityPtr->GetPlayerNickname(0);
		}
	}
	return "";
}

bool UECRGameInstance::GetIsLoggedIn()
{
	return UKismetSystemLibrary::IsLoggedIn(GetPrimaryPlayerController());
}

FString UECRGameInstance::GetUserAccountID()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			const FUniqueNetIdPtr UniqueNetIdPtr = OnlineIdentityPtr->GetUniquePlayerId(0);
			return UECROnlineSubsystem::NetIdToString(UniqueNetIdPtr);
		}
	}
	return "";
}

FString UECRGameInstance::GetUserAuthToken()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			return OnlineIdentityPtr->GetAuthToken(0);
		}
	}
	return "";
}

void UECRGameInstance::QueueGettingFriendsList()
{
	if (OnlineSubsystem)
	{
		if (IOnlineFriendsPtr Friends = OnlineSubsystem->GetFriendsInterface())
		{
			Friends->ReadFriendsList(
				0,TEXT(""),
				FOnReadFriendsListComplete::CreateUObject(this, &UECRGameInstance::OnReadFriendsListComplete)
			);
		}
	}
}

void UECRGameInstance::CreateParty(TMap<FString, FString> SessionData)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				if (GetIsInHostPartySession())
				{
					OnPartyCreationComplete(PARTY_LOBBY_SESSION_NAME, true);
				}
				else
				{
					// Setting custom party data (eg faction)
					FOnlineSessionSettings SessionSettings = GetPartySessionSettings();
					for (TTuple<FString, FString> DataTuple : SessionData)
					{
						SessionSettings.Set(FName{DataTuple.Key}, DataTuple.Value,
						                    EOnlineDataAdvertisementType::ViaOnlineService);
					}

					SessionInterface->CreateSession(0, PARTY_LOBBY_SESSION_NAME, SessionSettings);
					SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
					SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
						this, &UECRGameInstance::OnPartyCreationComplete);
				}
			}
		}
	}
}


void UECRGameInstance::DestroyParty()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnDestroySessionCompleteDelegates(this);
			OnlineSessionPtr->OnDestroySessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnDestroySessionComplete);
			OnlineSessionPtr->DestroySession(PARTY_LOBBY_SESSION_NAME);
		}
	}
}


bool UECRGameInstance::GetIsInHostPartySession()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				return SessionInterface->IsPlayerInSession(
					PARTY_LOBBY_SESSION_NAME, *Identity->GetUniquePlayerId(0).Get());
			}
		}
	}
	return false;
}

bool UECRGameInstance::GetIsInClientPartySession()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				if (SessionInterface->GetNamedSession(PARTY_LOBBY_CLIENT_SESSION_NAME))
				{
					return true;
				}
				// return SessionInterface->IsPlayerInSession(
				// 	PARTY_LOBBY_CLIENT_SESSION_NAME, *Identity->GetUniquePlayerId(0).Get());
			}
		}
	}
	return false;
}


FString UECRGameInstance::GetPartyMemberName(FUniqueNetIdRepl MemberId)
{
	if (!MemberId.IsValid())
	{
		return "";
	}

	if (OnlineSubsystem)
	{
		if (APlayerController* PC = GetPrimaryPlayerController())
		{
			if (APlayerState* PlayerState = PC->GetPlayerState<APlayerState>())
			{
				if (PlayerState->GetUniqueId() == MemberId)
				{
					return GetPlayerNickname();
				}
			}
		}

		if (const IOnlineFriendsPtr FriendInterface = OnlineSubsystem->GetFriendsInterface())
		{
			TSharedPtr<FOnlineFriend> OnlineFriend = FriendInterface->
				GetFriend(0, *MemberId.GetUniqueNetId().Get(), "");
			if (OnlineFriend)
			{
				return OnlineFriend->GetDisplayName();
			}
		}
	}
	return "";
}

void UECRGameInstance::KickPartyMember(FUniqueNetIdRepl MemberId)
{
	if (!MemberId.IsValid())
	{
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				SessionInterface->
					RemovePlayerFromSession(0, PARTY_LOBBY_SESSION_NAME, *MemberId.GetUniqueNetId().Get());
			}
		}
	}
}

void UECRGameInstance::LeaveParty()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				SessionInterface->DestroySession(
					PARTY_LOBBY_CLIENT_SESSION_NAME,
					FOnDestroySessionCompleteDelegate::CreateUObject(
						this, &UECRGameInstance::OnPartyLeaveComplete));
			}
		}
	}
}

void UECRGameInstance::InviteToParty(FUniqueNetIdRepl PlayerId)
{
	if (!PlayerId.IsValid())
	{
		return;
	}

	if (GetIsInClientPartySession())
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't invite another player to party, since in client party session"))
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				SessionInterface->SendSessionInviteToFriend(0, PARTY_LOBBY_SESSION_NAME,
				                                            *PlayerId.GetUniqueNetId().Get());
			}
		}
	}
}


TArray<FUniqueNetIdRepl> UECRGameInstance::GetPartyMembersList(bool bForClient)
{
	TArray<FUniqueNetIdRepl> Result;
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				FNamedOnlineSession* NamedOnlineSession = SessionInterface->GetNamedSession(
					bForClient ? PARTY_LOBBY_CLIENT_SESSION_NAME : PARTY_LOBBY_SESSION_NAME);
				if (NamedOnlineSession)
				{
					for (TSharedRef<FUniqueNetId const, ESPMode::ThreadSafe> Member : NamedOnlineSession->
					     RegisteredPlayers)
					{
						Result.Add(Member);
					}
				}
			}
		}
	}
	return Result;
}

bool UECRGameInstance::SetPartyData(FString Key, FString Value)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				FOnlineSessionSettings* OnlineSessionSettings = SessionInterface->GetSessionSettings(
					PARTY_LOBBY_SESSION_NAME);
				if (OnlineSessionSettings)
				{
					FOnlineSessionSettings OnlineSessionSettingsUnpacked = *OnlineSessionSettings;
					OnlineSessionSettingsUnpacked.
						Set(FName{Key}, Value, EOnlineDataAdvertisementType::ViaOnlineService);
					SessionInterface->UpdateSession(PARTY_LOBBY_SESSION_NAME, OnlineSessionSettingsUnpacked, true);
					return true;
				}
			}
		}
	}
	return false;
}

bool UECRGameInstance::SetPartyMemberData(FString Key, FString Value, bool bForClient)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				// Get the current session settings
				FOnlineSessionSettings* OnlineSessionSettings = SessionInterface->GetSessionSettings(
					bForClient ? PARTY_LOBBY_CLIENT_SESSION_NAME : PARTY_LOBBY_SESSION_NAME);

				if (OnlineSessionSettings)
				{
					// Get the player ID
					FUniqueNetIdRef PlayerId = Identity->GetUniquePlayerId(0)->AsShared();

					// Modify MemberSettings
					FSessionSettings& Tuples = OnlineSessionSettings->MemberSettings.FindOrAdd(PlayerId);

					FOnlineSessionSetting Setting = {Value, EOnlineDataAdvertisementType::ViaOnlineService};
					Tuples.Add(FName{Key}, Setting);

					// Update the session with modified settings
					SessionInterface->UpdateSession(
						bForClient ? PARTY_LOBBY_CLIENT_SESSION_NAME : PARTY_LOBBY_SESSION_NAME,
						*OnlineSessionSettings, true);

					return true;
				}
			}
		}
	}
	return false;
}

FString UECRGameInstance::GetPartyData(bool bForClient)
{
	if (OnlineSubsystem)
	{
		if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
		{
			FNamedOnlineSession* Session = SessionInterface->GetNamedSession(
				bForClient ? PARTY_LOBBY_CLIENT_SESSION_NAME : PARTY_LOBBY_SESSION_NAME);
			if (Session)
			{
				return UECROnlineSubsystem::ConvertSessionSettingsToJson(Session->SessionSettings);
			}
		}
	}
	return "";
}

void UECRGameInstance::StartListeningForPartyEvents()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			if (IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface())
			{
				// Accepting invite to party via overlay
				SessionInterface->ClearOnSessionUserInviteAcceptedDelegates(this);
				SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(
					FOnSessionUserInviteAcceptedDelegate::CreateUObject(
						this, &UECRGameInstance::OnPartyInviteAcceptedByMe));

				// Member changes
				SessionInterface->ClearOnSessionParticipantsChangeDelegates(this);
				SessionInterface->AddOnSessionParticipantsChangeDelegate_Handle(
					FOnSessionParticipantsChangeDelegate::CreateUObject(
						this, &UECRGameInstance::OnPartyMembersChanged));

				// Member data changes
				SessionInterface->ClearOnSessionParticipantSettingsUpdatedDelegates(this);
				SessionInterface->AddOnSessionParticipantSettingsUpdatedDelegate_Handle(
					FOnSessionParticipantSettingsUpdatedDelegate::CreateUObject(
						this, &UECRGameInstance::OnPartyMemberDataChanged));

				// Party data updates for clients
				SessionInterface->ClearOnSessionSettingsUpdatedDelegates(this);
				SessionInterface->AddOnSessionSettingsUpdatedDelegate_Handle(
					FOnSessionSettingsUpdatedDelegate::CreateUObject(this, &UECRGameInstance::OnPartyDataReceived));

				// Session failure
				SessionInterface->ClearOnSessionFailureDelegates(this);
				SessionInterface->AddOnSessionFailureDelegate_Handle(
					FOnSessionFailureDelegate::CreateUObject(this, &UECRGameInstance::OnSessionFailure));
			}
		}
	}
}


void UECRGameInstance::Init()
{
	Super::Init();
}


void UECRGameInstance::Shutdown()
{
	Super::Shutdown();
}
