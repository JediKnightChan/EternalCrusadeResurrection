// Copyleft: All rights reversed


#include "Online/ECROnlineSubsystem.h"
#include "System/MatchSettings.h"


FECRMatchResult::FECRMatchResult()
{
	CurrentPlayerAmount = 0;
	MatchStartedTimestamp = 0.0f;
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

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_REGION, StringBuffer);
	Region = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_WEATHER_NAME, StringBuffer);
	Weather = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_DAYTIME_NAME, StringBuffer);
	DayTime = FName{*StringBuffer};

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_CURRENT_PLAYER_AMOUNT, CurrentPlayerAmount);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_STARTED_TIME, MatchStartedTimestamp);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_FACTIONS, FactionsString);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_USER_DISPLAY_NAME, UserDisplayName);
}


FECRMatchSettings::FECRMatchSettings()
{
}


FString UECROnlineSubsystem::NetIdToString(const FUniqueNetIdRepl NetId)
{
	return NetId.ToString();
}
