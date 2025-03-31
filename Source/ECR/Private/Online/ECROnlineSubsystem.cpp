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

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_IN_GAME_UNIQUE_ID_FOR_SEARCH, StringBuffer);
	InGameUniqueIdForSearch = StringBuffer;

	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_CURRENT_PLAYER_AMOUNT, CurrentPlayerAmount);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_STARTED_TIME, MatchStartedTimestamp);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_FACTIONS, FactionsString);
	BlueprintSession.OnlineResult.Session.SessionSettings.Get(SETTING_USER_DISPLAY_NAME, UserDisplayName);
}


FECRMatchSettings::FECRMatchSettings()
{
	MatchStartedTime = 0.0f;
	TimeDelta = 0.0f;
	CurrentPlayerAmount = 0;
}


FString UECROnlineSubsystem::NetIdToString(const FUniqueNetIdRepl NetId)
{
	return NetId.ToString();
}


FUniqueNetIdRepl UECROnlineSubsystem::StringToNetId(const FString& String)
{
	FUniqueNetIdRepl NetId;
	NetId.FromJson(String);
	return NetId;
}


FString UECROnlineSubsystem::ConvertSessionSettingsToJson(const FOnlineSessionSettings& Settings)
{
	TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	for (TTuple<FName, FOnlineSessionSetting> SettingTuple : Settings.Settings)
	{
		FString OutValue;
		SettingTuple.Value.Data.GetValue(OutValue);
		JsonObject->SetStringField(SettingTuple.Key.ToString(), OutValue);
	}

	TSharedRef<FJsonObject> PlayersSettingsJson = MakeShared<FJsonObject>();
	for (TTuple<FUniqueNetIdRef, FSessionSettings> PlayerSettingsTuple : Settings.MemberSettings)
	{
		TSharedRef<FJsonObject> PlayerSettingsJson = MakeShared<FJsonObject>();
		for (TTuple<FName, FOnlineSessionSetting> SettingTuple : PlayerSettingsTuple.Value)
		{
			FString OutValue;
			SettingTuple.Value.Data.GetValue(OutValue);
			PlayerSettingsJson->SetStringField(SettingTuple.Key.ToString(), OutValue);
		}
		PlayersSettingsJson->SetObjectField(PlayerSettingsTuple.Key.Get().ToString(), PlayerSettingsJson);
	}

	JsonObject->SetObjectField("members", PlayersSettingsJson);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (FJsonSerializer::Serialize(JsonObject, Writer))
	{
		Writer->Close();
	}
	return JsonString;
}


FString UECROnlineSubsystem::GetUniqueGuidString()
{
	const FGuid Guid = FGuid::NewGuid();
	return Guid.ToString(EGuidFormats::DigitsWithHyphensLower);
}
