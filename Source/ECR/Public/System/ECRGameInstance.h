// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Online/ECROnlineSubsystem.h"
#include "ECRGameInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFriendListUpdated, bool, bSuccess, const TArray<FECRFriendData>&,
                                             Results);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyCreationFinished, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyJoinFinished, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyLeaveFinished, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyMemberRemoved, FUniqueNetIdRepl, Player);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyMembersChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyDataUpdated, FString, UpdatedJsonString);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnectedFromSession);

/**
 * 
 */
UCLASS()
class ECR_API UECRGameInstance : public UGameInstance
{
	GENERATED_BODY()

	/** Name assigned to player that will be shown in matches */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess = "true"))
	FString UserDisplayName;

	/** Match settings, not only for creation, but also real time updates */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FECRMatchSettings MatchCreationSettings;

	/** Online subsystem */
	class IOnlineSubsystem* OnlineSubsystem;

	/** Whether user is logged in */
	bool bDeprecatedIsLoggedIn;

	/** Session search object */
	TSharedPtr<class FOnlineSessionSearch> SessionSearchSettings;

	/** Get factions string (like "SM, Eldar vs CSM") **/
	static FString GetMatchFactionString(const TArray<FFactionAlliance>& FactionAlliances,
	                                     const TMap<FName, FText>& FactionNamesToShortTexts);

	/** Broadcaster for friend list update events */
	UPROPERTY(BlueprintAssignable)
	FOnFriendListUpdated OnFriendListUpdated_BP;

	/** Broadcaster for party creation events */
	UPROPERTY(BlueprintAssignable)
	FOnPartyCreationFinished OnPartyCreationFinished_BP;

	/** Broadcaster for party joins result */
	UPROPERTY(BlueprintAssignable)
	FOnPartyJoinFinished OnPartyJoinFinished_BP;

	/** Broadcaster for changes in the party */
	UPROPERTY(BlueprintAssignable)
	FOnPartyMembersChanged OnPartyMembersChanged_BP;

	/** Broadcaster for changes in the party data */
	UPROPERTY(BlueprintAssignable)
	FOnPartyDataUpdated OnPartyDataUpdated_BP;

	/** Broadcaster for leaving party */
	UPROPERTY(BlueprintAssignable)
	FOnPartyLeaveFinished OnPartyLeaveFinished_BP;

	/** Broadcaster for player removals */
	UPROPERTY(BlueprintAssignable)
	FOnPartyMemberRemoved OnPartyMemberRemoved_BP;
	
	/** Broadcaster for being disconnected from session */
	UPROPERTY(BlueprintAssignable)
	FOnDisconnectedFromSession OnDisconnectedFromSession_BP;

protected:
	/** Login via selected login type */
	void Login(FString PlayerName, FString LoginType, FString Id = "", FString Token = "");

	/** When OnLoginComplete fires, show main menu if success, or show error with GUISupervisor */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** On logout event */
	void OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful);

	/** When OnCreateMatchComplete fires, save match creation parameters and travel to match map */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);

	/** When OnFindSessionsComplete fires, pass matches data to GUISupervisor */
	void OnFindMatchesComplete(bool bWasSuccessful);

	/** When OnFindSessionsComplete fires, pass match data to GUISupervisor */
	void OnFindMatchByUniqueIdComplete(bool bWasSuccessful);

	/** When OnJoinSessionComplete fires, travel to the session map */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** When OnDestroySessionComplete fires, clear other delegates */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate to complete friends list read */
	void OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
	                               const FString& ErrorStr);

	/** Delegate to complete party creation */
	void OnPartyCreationComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate for party invite accept */
	void OnPartyInviteAcceptedByMe(const bool bWasSuccessful, const int32 ControllerId,
	                               FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult);

	/** Delegate to complete party joining */
	void OnJoinPartyComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Delegate to complete party leave */
	void OnPartyLeaveComplete(FName SessionName, bool bSuccess);

	/** Delegate for party join events */
	void OnPartyMembersChanged(FName SessionName, const FUniqueNetId& UniqueId, bool bJoined);

	/** Delegate for party member settings changes */
	void OnPartyMemberDataChanged(FName SessionName, const FUniqueNetId& TargetUniqueNetId,
	                              const FOnlineSessionSettings& SessionSettings);

	/** Delegate for party member removals */
	void OnPartyMemberRemoved(FName SessionName, const FUniqueNetId& Player);

	/** Delegate for party data changes */
	void OnPartyDataReceived(FName SessionName, const FOnlineSessionSettings& NewSettings);

	/** Delegate for session failures */
	void OnSessionFailure(const FUniqueNetId& PlayerId, ESessionFailure::Type Reason);

	/** Retrieve session settings for match */
	FOnlineSessionSettings GetSessionSettings();

	/** Retrieve session settings for party lobby */
	FOnlineSessionSettings GetPartySessionSettings();

public:
	UECRGameInstance();

	// Login functionality

	/** Log Out */
	UFUNCTION(BlueprintCallable)
	void LogOut();

	/** Login user via Epic Account */
	UFUNCTION(BlueprintCallable)
	void LoginViaEpic(FString PlayerName);

	/** Login user via Device ID */
	UFUNCTION(BlueprintCallable)
	void LoginPersistent(FString PlayerName);

	/** Login user via DevTool */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevTool(FString PlayerName, FString Address, FString CredName);

	// Match functionality

	/** Create match, by player (P2P) */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const FString GameVersion, const FString InGameUniqueIdForSearch, const FName ModeName,
	                 const FName MapName, const FString MapPath, const FName MissionName,
	                 const FName RegionName, const double TimeDelta, const FName WeatherName,
	                 const FName DayTimeName, const TArray<FFactionAlliance> Alliances, const TMap<FName, int32>
	                 FactionNamesToCapacities, const TMap<FName, FText> FactionNamesToShortTexts);

	/** Find matches */
	UFUNCTION(BlueprintCallable)
	void FindMatches(const FString GameVersion = "", const FString MatchType = "",
	                 const FString MatchMode = "", const FString MapName = "", const FString RegionName = "");

	/** Find match by unique match id assigned by the game (used in parties custom implementation logic) */
	UFUNCTION(BlueprintCallable)
	void FindMatchByUniqueInGameId(const FString GameVersion = "", const FString MatchId = "");

	/** Join match */
	UFUNCTION(BlueprintCallable)
	void JoinMatch(FBlueprintSessionResult Session);

	/** Register or unregister player in match */
	UFUNCTION(BlueprintCallable)
	bool TogglePlayerRegistrationInMatch(FUniqueNetIdRepl Player, bool bRegister);

	/** Updates default session (match) settings after modifying them in MatchCreationSettings property */
	UFUNCTION(BlueprintCallable)
	void UpdateSessionSettings();

	/** Update current player amount */
	UFUNCTION(BlueprintCallable)
	void UpdateSessionCurrentPlayerAmount(int32 NewPlayerAmount);

	/** Update match started timestamp */
	UFUNCTION(BlueprintCallable)
	void UpdateSessionMatchStartedTimestamp(double NewTimestamp);

	/** Update current player amount */
	UFUNCTION(BlueprintCallable)
	void UpdateSessionDayTime(FName NewDayTime);

	/** Leave match */
	UFUNCTION(BlueprintCallable)
	void DestroySession();

	/** Get whether the player is logged in */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool GetDeprecatedIsLoggedIn() const { return bDeprecatedIsLoggedIn; }

	// Identity functionality

	/** Get player nickname */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPlayerNickname();

	/** Get if player is logged in */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetIsLoggedIn();

	/** Get player account id */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetUserAccountID();

	/** Get player account auth token */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetUserAuthToken();

	/** Queues retrieving friends list from online sybsystem */
	UFUNCTION(BlueprintCallable)
	void QueueGettingFriendsList();

	// Party functionality

	/** Create party */
	UFUNCTION(BlueprintCallable)
	void CreateParty(TMap<FString, FString> SessionData);

	/** Destroy party lobby */
	UFUNCTION(BlueprintCallable)
	void DestroyParty();

	/** Check if is hosting a party (should always be if no errors) */
	UFUNCTION(BlueprintCallable)
	bool GetIsInHostPartySession();

	/** Check if is in other player's party */
	UFUNCTION(BlueprintCallable)
	bool GetIsInClientPartySession();

	/** Get name of party member (WARNING: only works for myself or friends, better use custom attribute in member settings) */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPartyMemberName(FUniqueNetIdRepl MemberId);

	/* Kick party member */
	UFUNCTION(BlueprintCallable)
	void KickPartyMember(FUniqueNetIdRepl MemberId);

	/** Leave party (as client) */
	UFUNCTION(BlueprintCallable)
	void LeaveParty();

	/** Invite player to party */
	UFUNCTION(BlueprintCallable)
	void InviteToParty(FUniqueNetIdRepl PlayerId);

	/** Retrieves list of players in the party */
	UFUNCTION(BlueprintCallable)
	TArray<FUniqueNetIdRepl> GetPartyMembersList(bool bForClient);

	/** Sets custom attribute on party */
	UFUNCTION(BlueprintCallable)
	bool SetPartyData(FString Key, FString Value);

	/** Toggles whether party uses presence (online subsystem term) or not */
	UFUNCTION(BlueprintCallable)
	bool TogglePartyPresence(bool bWantPresence);

	/** Sets party member (me only) custom attribute to value */
	UFUNCTION(BlueprintCallable)
	bool SetPartyMemberData(FString Key, FString Value, bool bForClient);

	/** Sets party member (me only) custom attributes to values */
	UFUNCTION(BlueprintCallable)
	bool SetPartyMemberDataInBatch(TMap<FString, FString> Data, bool bForClient);

	/** Returns JSON serialized party data */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPartyData(bool bForClient);

	/** Subscribing for delegates related to parties */
	UFUNCTION(BlueprintCallable)
	void StartListeningForPartyEvents();

public:
	virtual void Init() override;

	virtual void Shutdown() override;
};
