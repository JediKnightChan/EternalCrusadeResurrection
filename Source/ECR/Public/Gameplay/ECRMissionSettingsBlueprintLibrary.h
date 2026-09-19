#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ECRMissionSettingsBlueprintLibrary.generated.h"


UENUM(BlueprintType)
enum class EMissionSettingType : uint8
{
    NotSupported,
    Bool,
    Int,
    Float
};


USTRUCT(BlueprintType)
struct FMissionSettingDefinition
{
    GENERATED_BODY()

    // Actual Blueprint variable name, e.g. UI_RespawnTime
    UPROPERTY(BlueprintReadWrite)
    FName PropertyName;

    // Name shown in UI, e.g. Respawn Time
    UPROPERTY(BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite)
    EMissionSettingType Type = EMissionSettingType::Bool;

    // If non-empty, UI should normally use a dropdown.
    UPROPERTY(BlueprintReadWrite)
    TArray<FString> Choices;

    // Current default value from the Blueprint CDO.
    UPROPERTY(BlueprintReadWrite)
    FString DefaultValue;
};


USTRUCT(BlueprintType)
struct FMissionSettingValue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FName PropertyName;

    // Stored as string so the same structure can contain any other type like bool/int/float.
    UPROPERTY(BlueprintReadWrite)
    FString Value;
};


USTRUCT(BlueprintType)
struct FMissionSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<FMissionSettingValue> Values;
};


/** This class is a very basic reflection system around customizable variables in blueprint MasterGameplayMission */
UCLASS()
class ECR_API UECRMissionSettingsBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /*
     * Finds all Blueprint variables beginning with UI_
     * and returns their definitions.
     *
     * Pass the mission Blueprint class.
     */
    UFUNCTION(BlueprintCallable, Category="Mission Settings")
    static TArray<FMissionSettingDefinition> GetMissionSettingDefinitions(
        UClass* MissionClass
    );


    /*
     * Reads the current value of a setting from a specific
     * mission instance.
     */
    UFUNCTION(BlueprintCallable, Category="Mission Settings")
    static bool GetMissionSettingValue(
        UObject* MissionInstance,
        FName PropertyName,
        FString& OutValue
    );


    /*
     * Applies saved settings to a specific mission instance.
     */
    UFUNCTION(BlueprintCallable, Category="Mission Settings")
    static bool ApplyMissionSettings(
        UObject* MissionInstance,
        const FMissionSettings& Settings
    );


    /*
     * Convenience function for creating/updating a stored setting.
     */
    UFUNCTION(BlueprintCallable, Category="Mission Settings")
    static void SetStoredMissionSetting(
        UPARAM(ref) FMissionSettings& Settings,
        FName PropertyName,
        FString Value
    );
};