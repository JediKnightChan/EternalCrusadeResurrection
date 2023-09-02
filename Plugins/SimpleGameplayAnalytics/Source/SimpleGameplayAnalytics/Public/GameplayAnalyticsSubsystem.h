// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayAnalyticsSubsystem.generated.h"

/** Alliance of factions (eg LSM & Eldar) */
USTRUCT(BlueprintType)
struct FGameplayAnalyticsEventData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TMap<FString, FString> EventData;

	TSharedPtr<FJsonObject> ToJson();
};

/**
 * GameInstance Subsystem handling gameplay analytics (via recording gameplay events with attributes,
 * eg event "Damage" with attributes: {"Type": "Damage", "Value": "22.0", "Instigator": "0", "Target": "1",
 * "InstigatorClass": "HeavyInfantry", "TargetClass": "LightInfantry"}).
 */
UCLASS()
class SIMPLEGAMEPLAYANALYTICS_API UGameplayAnalyticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	UGameplayAnalyticsSubsystem();

protected:
	UFUNCTION(BlueprintCallable, Category="Gameplay Analytics")
	void ClearAllData();

	/** Add new event */
	UFUNCTION(BlueprintCallable, Category="Gameplay Analytics")
	void AddEvent(FString EventType, TMap<FString, FString> EventData);

	/** Retrieve recorded events */
	UFUNCTION(BlueprintCallable, Category="Gameplay Analytics")
	TArray<FGameplayAnalyticsEventData> RetrieveEvents();

	UFUNCTION(BlueprintCallable, Category="Gameplay Analytics")
	FString RetrieveEventsAsJsonString();
private:
	/** Array of events, which are maps of strings to strings. Because it's array, can be later converted to table */
	TArray<FGameplayAnalyticsEventData> AnalyticsData;
};
