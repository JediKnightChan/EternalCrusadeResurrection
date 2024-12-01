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
	int32 Strength = 0;
};


/** Data about match that will be available in match search */
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
	FName Region;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Weather;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName DayTime;

	/** For some reason getting int in EOS doesn't work */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString CurrentPlayerAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double MatchStartedTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString FactionsString;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString UserDisplayName;
};


/** Friend data */
USTRUCT(BlueprintType)
struct FECRFriendData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsPlayingThisGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RealName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsJoinAble;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FUniqueNetIdRepl CurrentSessionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FUniqueNetIdRepl UserId;
};

/** Data about match that will be saved in game instance to be available after match creation */
USTRUCT(BlueprintType)
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
struct FECRMatchSettings
{
	GENERATED_BODY()

	// Default constructor
	FECRMatchSettings();

	// Real constructor
	FECRMatchSettings(const FString& GameVersion, const FName& GameMode, const FName& MapName, const FString& MapPath,
	                  const FName& GameMission,
	                  const FName& Region, const FName& WeatherName, const FName& DayTimeName, const double TimeDelta,
	                  const TArray<FFactionAlliance>& Alliances,
	                  const TMap<FName, int32>& FactionNamesToCapacities,
	                  const TMap<FName, FText>& FactionNamesToShortTexts)
		: GameVersion(GameVersion),
		  GameMode(GameMode),
		  MapName(MapName),
		  MapPath(MapPath),
		  GameMission(GameMission),
		  Region(Region),
		  WeatherName(WeatherName),
		  DayTimeName(DayTimeName),
		  TimeDelta(TimeDelta),
		  Alliances(Alliances),
		  FactionNamesToCapacities(FactionNamesToCapacities),
		  FactionNamesToShortTexts(FactionNamesToShortTexts)
	{
		CurrentPlayerAmount = 1;
		MatchStartedTime = 0.0f;
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString GameVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName GameMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName MapName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MapPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName GameMission;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName Region;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName WeatherName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FName DayTimeName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double TimeDelta;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FFactionAlliance> Alliances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, int32> FactionNamesToCapacities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FName, FText> FactionNamesToShortTexts;

	// Updatable stats

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CurrentPlayerAmount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	double MatchStartedTime;
};


/**
 * GameInstance Subsystem previously handling online functionality (logging in, match creation and joining, ...),
 * which is now moved to GameInstance to create sessions on dedicated server on GameInstance->Init
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API UECROnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Convert NetID to string */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FString NetIdToString(FUniqueNetIdRepl NetId);
};
