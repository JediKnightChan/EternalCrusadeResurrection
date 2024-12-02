// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Online/ECROnlineSubsystem.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "ECRGameInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFriendListUpdated, bool, bSuccess, const TArray<FECRFriendData>&,
                                             Results);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyCreationFinished, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartyMemberKickFinished, bool, bSuccess, FUniqueNetIdRepl, Member);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPartyInvitationReceived, FUniqueNetIdRepl, SourceId, FString,
                                             SourceDisplayName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyJoinFinished, bool, bSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPartyMembersChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyDataUpdated, FString, UpdatedJsonString);

/**
 * 
 */
UCLASS()
class ECR_API UECRGameInstance : public UGameInstance
{
	GENERATED_BODY()

	/** Name assigned to player that will be shown in matches */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString UserDisplayName;

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

	/** Broadcaster for party kick events */
	UPROPERTY(BlueprintAssignable)
	FOnPartyMemberKickFinished OnPartyMemberKickFinished_BP;

	/** Broadcaster for party invites */
	UPROPERTY(BlueprintAssignable)
	FOnPartyInvitationReceived OnPartyInviteReceived_BP;

	/** Broadcaster for party joins result */
	UPROPERTY(BlueprintAssignable)
	FOnPartyJoinFinished OnPartyJoinFinished_BP;

	/** Broadcaster for changes in the party */
	UPROPERTY(BlueprintAssignable)
	FOnPartyMembersChanged OnPartyMembersChanged_BP;

	/** Broadcaster for changes in the party data */
	UPROPERTY(BlueprintAssignable)
	FOnPartyDataUpdated OnPartyDataUpdated_BP;

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

	/** When OnJoinSessionComplete fires, travel to the session map */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** When OnDestroySessionComplete fires, clear other delegates */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate to complete friends list read */
	void OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName,
	                               const FString& ErrorStr);

	/** Delegate to complete party creation */
	void OnPartyCreationComplete(FName SessionName, bool bWasSuccessful);

	/** Delegate to complete party member kick */
	void OnKickPartyMemberComplete(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId,
	                               const FUniqueNetId& MemberId, const EKickMemberCompletionResult Result);

	/** Delegate for receiving party invites */
	void OnPartyInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId,
	                           const FOnlineSessionSearchResult& InviteResult);

	/** Delegate to complete party joining */
	void OnJoinPartyComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** Delegate to complete party leave */
	void OnPartyLeaveComplete(FName SessionName, bool bSuccess);

	/** Delegate for party join events */
	void OnPartyMemberJoined(FName SessionName, const FUniqueNetId& UniqueId, bool bJoined);

	/** Delegate for party left events */
	void OnPartyMemberLeft(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId);

	/** Delegate for party data changes */
	void OnPartyDataReceived(FName SessionName, const FOnlineSessionSettings& NewSettings);

	void OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const TArray<FOnlineSessionSearchResult>& SearchResult);
	
	FOnlineSessionSettings GetSessionSettings();

	FOnlineSessionSettings GetPartySessionSettings();

public:
	UECRGameInstance();

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

	/** Create match, by player (P2P) */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const FString GameVersion, const FName ModeName,
	                 const FName MapName, const FString MapPath, const FName MissionName,
	                 const FName RegionName, const double TimeDelta, const FName WeatherName,
	                 const FName DayTimeName, const TArray<FFactionAlliance> Alliances, const TMap<FName, int32>
	                 FactionNamesToCapacities, const TMap<FName, FText> FactionNamesToShortTexts);

	/** Find matches */
	UFUNCTION(BlueprintCallable)
	void FindMatches(const FString GameVersion = "", const FString MatchType = "",
	                 const FString MatchMode = "", const FString MapName = "", const FString RegionName = "");

	/** Join match */
	UFUNCTION(BlueprintCallable)
	void JoinMatch(FBlueprintSessionResult Session);

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

	UFUNCTION(BlueprintCallable)
	void QueueGettingFriendsList();

	// Party functionality

	/** Create party */
	UFUNCTION(BlueprintCallable)
	void CreateParty();

	/** Check if in party */
	UFUNCTION(BlueprintCallable)
	bool GetIsInParty();

	/** Check if is party leader */
	UFUNCTION(BlueprintCallable)
	bool GetIsPartyLeader();

	/** Get name of party member */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPartyMemberName(FUniqueNetIdRepl MemberId);

	/* Kick party member */
	UFUNCTION(BlueprintCallable)
	void KickPartyMember(FUniqueNetIdRepl MemberId);

	/** Leave party */
	UFUNCTION(BlueprintCallable)
	void LeaveParty();

	/** Invite player to party */
	UFUNCTION(BlueprintCallable)
	void InviteToParty(FUniqueNetIdRepl PlayerId);

	/** Accept party invite */
	UFUNCTION(BlueprintCallable)
	void AcceptPartyInvite(FUniqueNetIdRepl PlayerId);

	/** Decline party invite */
	UFUNCTION(BlueprintCallable)
	void DeclinePartyInvite(FUniqueNetIdRepl PlayerId);

	UFUNCTION(BlueprintCallable)
	TArray<FECRPartyMemberData> GetPartyMembersList();

	UFUNCTION(BlueprintCallable)
	bool SetPartyData(FString Key, FString Value);

	UFUNCTION(BlueprintCallable)
	void StartListeningForPartyEvents();

public:
	virtual void Init() override;

	virtual void Shutdown() override;
};
