// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FindSessionsCallbackProxy.h"
#include "ECROnlineSubsystem.generated.h"


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
	                  const TMap<FName, int32>& FactionNamesToSides, const TMap<FName, int32>& FactionNamesToCapacities)
		: GameMode(GameMode),
		  MapName(MapName),
		  MapPath(MapPath),
		  GameMission(GameMission),
		  FactionNamesToSides(FactionNamesToSides),
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
	TMap<FName, int32> FactionNamesToSides;

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
	static FString GetMatchFactionString(const TMap<FName, int32>& FactionNamesToSides,
	                                     const TMap<FName, FText>& FactionNamesToShortTexts);
protected:
	/** Login via selected login type */
	void Login(FString PlayerName, FString LoginType);

	/** When OnLoginComplete fires, show main menu if success, or show error with GUISupervisor */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** When OnCreateMatchComplete fires, save match creation parameters and travel to match map */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);

	/** When OnFindSessionsComplete fires, pass matches data to GUISupervisor */
	void OnFindMatchesComplete(bool bWasSuccessful);

	/** When OnJoinSessionComplete fires, travel to the session map */
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

public:
	UECROnlineSubsystem();

	/** Login user via Epic Account */
	UFUNCTION(BlueprintCallable)
	void LoginViaEpic(FString PlayerName);

	/** Login user via Device ID */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevice(FString PlayerName);

	/** Create match, by player (P2P) */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const FName ModeName,
	                 const FName MapName, const FString MapPath, const FName MissionName,
	                 const TMap<FName, int32> FactionNamesToSides, const TMap<FName, int32>
	                 FactionNamesToCapacities, const TMap<FName, FText> FactionNamesToShortTexts);

	/** Create match, by player (P2P) or dedicated server */
	UFUNCTION(BlueprintCallable)
	void FindMatches(const FString MatchType = "",
	                 const FString MatchMode = "", const FString MapName = "");
	
	/** Join match */
	UFUNCTION(BlueprintCallable)
	void JoinMatch(FBlueprintSessionResult Session);
};
