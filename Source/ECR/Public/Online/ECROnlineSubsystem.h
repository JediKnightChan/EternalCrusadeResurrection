// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FindSessionsCallbackProxy.h"
#include "ECROnlineSubsystem.generated.h"


/** Alliance of factions (eg LSM & Eldar) */
USTRUCT(BlueprintType)
struct FFactionAlliance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> FactionNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Strength;
};


USTRUCT(BlueprintType)
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
struct FECRMatchResult
{
	GENERATED_BODY()

	/** Default constructor */
	FECRMatchResult();

	/** Real constructor */
	explicit FECRMatchResult(const FBlueprintSessionResult BlueprintSessionIn);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FBlueprintSessionResult BlueprintSession;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Map;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Mode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Mission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FactionsString;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString UserDisplayName;
};


USTRUCT(BlueprintType)
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
struct FECRMatchSettings
{
	GENERATED_BODY()

	// Default constructor
	FECRMatchSettings();

	// Real constructor
	FECRMatchSettings(const FName& GameMode, const FName& MapName, const FString& MapPath, const FName& GameMission,
	                  const TArray<FFactionAlliance>& Alliances, const TMap<FName, int32>& FactionNamesToCapacities)
		: GameMode(GameMode),
		  MapName(MapName),
		  MapPath(MapPath),
		  GameMission(GameMission),
		  Alliances(Alliances),
		  FactionNamesToCapacities(FactionNamesToCapacities)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName GameMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName MapName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MapPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName GameMission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FFactionAlliance> Alliances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, int32> FactionNamesToCapacities;
};


/**
 * GameInstance Subsystem handling online functionality (logging in, match creation and joining, ...)
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API UECROnlineSubsystem : public UGameInstanceSubsystem
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
	bool bIsLoggedIn;

	/** Session search object */
	TSharedPtr<class FOnlineSessionSearch> SessionSearchSettings;

	/** Get factions string (like "SM, Eldar vs CSM") **/
	static FString GetMatchFactionString(const TArray<FFactionAlliance>& FactionAlliances,
	                                     const TMap<FName, FText>& FactionNamesToShortTexts);
protected:
	/** Login via selected login type */
	void Login(FString PlayerName, FString LoginType, FString Id = "", FString Token = "");

	/** When OnLoginComplete fires, show main menu if success, or show error with GUISupervisor */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** When OnCreateMatchComplete fires, save match creation parameters and travel to match map */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);

	/** When OnFindSessionsComplete fires, pass matches data to GUISupervisor */
	void OnFindMatchesComplete(bool bWasSuccessful);

	/** When OnJoinSessionComplete fires, travel to the session map */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	/** When OnDestroySessionComplete fires, clear other delegates */
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
public:
	UECROnlineSubsystem();

	/** Login user via Epic Account */
	UFUNCTION(BlueprintCallable)
	void LoginViaEpic(FString PlayerName);

	/** Login user via Device ID */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevice(FString PlayerName);

	/** Login user via DevTool */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevTool(FString PlayerName, FString Address, FString CredName);

	/** Create match, by player (P2P) */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const FName ModeName,
	                 const FName MapName, const FString MapPath, const FName MissionName,
	                 const TArray<FFactionAlliance> Alliances, const TMap<FName, int32>
	                 FactionNamesToCapacities, const TMap<FName, FText> FactionNamesToShortTexts);

	/** Create match, by player (P2P) or dedicated server */
	UFUNCTION(BlueprintCallable)
	void FindMatches(const FString MatchType = "",
	                 const FString MatchMode = "", const FString MapName = "");

	/** Join match */
	UFUNCTION(BlueprintCallable)
	void JoinMatch(FBlueprintSessionResult Session);

	/** Leave match */
	UFUNCTION(BlueprintCallable)
	void DestroySession();

	/** Get whether the player is logged in */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FORCEINLINE bool GetIsLoggedIn() const { return bIsLoggedIn; }

	/** Get player nickname */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPlayerNickname();

	/** Convert NetID to string */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FString NetIdToString(FUniqueNetIdRepl NetId);
};
