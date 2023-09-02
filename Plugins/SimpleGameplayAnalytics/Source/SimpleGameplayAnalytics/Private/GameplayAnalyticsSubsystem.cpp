#include "GameplayAnalyticsSubsystem.h"
#include "Json/Public/Dom/JsonObject.h"
#include "Json/Public/Dom/JsonValue.h"

TSharedPtr<FJsonObject> FGameplayAnalyticsEventData::ToJson()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	for (auto [Key, Value] : EventData)
	{
		JsonObject->SetStringField(Key, Value);
	}
	return JsonObject;
}

UGameplayAnalyticsSubsystem::UGameplayAnalyticsSubsystem()
{
	AnalyticsData = {};
}

void UGameplayAnalyticsSubsystem::ClearAllData()
{
	AnalyticsData.Empty();
}

void UGameplayAnalyticsSubsystem::AddEvent(const FString EventType, TMap<FString, FString> EventData)
{
	EventData.Add("Type", EventType);

	const FGameplayAnalyticsEventData DataContainer = FGameplayAnalyticsEventData{EventData};
	AnalyticsData.Add(DataContainer);
}

TArray<FGameplayAnalyticsEventData> UGameplayAnalyticsSubsystem::RetrieveEvents()
{
	return AnalyticsData;
}

FString UGameplayAnalyticsSubsystem::RetrieveEventsAsJsonString()
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	TArray<TSharedPtr<FJsonValue>> Array;
	for (FGameplayAnalyticsEventData EventData : AnalyticsData)
	{
		TSharedPtr<FJsonObject> Item = EventData.ToJson();
		Array.Add(MakeShareable(new FJsonValueObject(Item)));
	}

	JsonObject->SetArrayField("data", Array);

	FString OutputString;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}
