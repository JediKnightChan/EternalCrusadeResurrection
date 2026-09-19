#include "Gameplay/ECRMissionSettingsBlueprintLibrary.h"
#include "JsonObjectConverter.h"
#include "UObject/UnrealType.h"


namespace MissionSettingsConstants
{
    static const FString SettingPrefix = TEXT("UI_");
    static const FString ChoicePrefix = TEXT("UIChoices_");
}


// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

static bool IsSettingProperty(const FProperty* Property)
{
    if (!Property)
    {
        return false;
    }

    const FString Name = Property->GetName();

    // UIChoices_X are helper variables, not settings.
    if (Name.StartsWith(MissionSettingsConstants::ChoicePrefix))
    {
        return false;
    }

    return Name.StartsWith(MissionSettingsConstants::SettingPrefix);
}


static EMissionSettingType GetSettingType(
    const FProperty* Property)
{
    if (CastField<FBoolProperty>(Property))
    {
        return EMissionSettingType::Bool;
    }

    if (CastField<FIntProperty>(Property))
    {
        return EMissionSettingType::Int;
    }

    if (CastField<FDoubleProperty>(Property))
    {
        return EMissionSettingType::Float;
    }

    return EMissionSettingType::NotSupported;
}


static bool IsSupportedSettingType(
    const FProperty* Property)
{
    return
        CastField<FBoolProperty>(Property) ||
        CastField<FIntProperty>(Property) ||
        CastField<FDoubleProperty>(Property);
}


static FString GetDisplayName(
    const FProperty* Property)
{
    FString Name = Property->GetName();

    Name.RemoveFromStart(
        MissionSettingsConstants::SettingPrefix
    );

    // Simple CamelCase → spaced name.
    FString Result;

    for (int32 i = 0; i < Name.Len(); ++i)
    {
        const TCHAR Character = Name[i];

        if (i > 0 && FChar::IsUpper(Character))
        {
            Result += TEXT(" ");
        }

        Result += Character;
    }

    return Result;
}


// ------------------------------------------------------------
// Read value
// ------------------------------------------------------------

static bool ReadPropertyValue(
    UObject* Object,
    FProperty* Property,
    FString& OutValue)
{
    if (FBoolProperty* BoolProperty =
        CastField<FBoolProperty>(Property))
    {
        const bool Value =
            BoolProperty->GetPropertyValue_InContainer(Object);

        OutValue = Value ? TEXT("true") : TEXT("false");

        return true;
    }

    if (FIntProperty* IntProperty =
        CastField<FIntProperty>(Property))
    {
        const int32 Value =
            IntProperty->GetPropertyValue_InContainer(Object);

        OutValue = LexToString(Value);

        return true;
    }

    if (FDoubleProperty* FloatProperty =
        CastField<FDoubleProperty>(Property))
    {
        const double Value =
            FloatProperty->GetPropertyValue_InContainer(Object);

        OutValue = LexToString(Value);

        return true;
    }

    return false;
}


// ------------------------------------------------------------
// Set value
// ------------------------------------------------------------

static bool WritePropertyValue(
    UObject* Object,
    FProperty* Property,
    const FString& Value)
{
    if (FBoolProperty* BoolProperty =
        CastField<FBoolProperty>(Property))
    {
        const bool NewValue =
            Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) ||
            Value.Equals(TEXT("1"));

        BoolProperty->SetPropertyValue_InContainer(
            Object,
            NewValue
        );

        return true;
    }

    if (FIntProperty* IntProperty =
        CastField<FIntProperty>(Property))
    {
        int32 NewValue;

        if (!LexTryParseString(NewValue, *Value))
        {
            return false;
        }

        IntProperty->SetPropertyValue_InContainer(
            Object,
            NewValue
        );

        return true;
    }

    if (FDoubleProperty* FloatProperty =
        CastField<FDoubleProperty>(Property))
    {
        double NewValue;

        if (!LexTryParseString(NewValue, *Value))
        {
            return false;
        }

        FloatProperty->SetPropertyValue_InContainer(
            Object,
            NewValue
        );

        return true;
    }

    // Generic JSON fallback
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Value);
    TSharedPtr<FJsonObject> RootJsonObject;

    if (!FJsonSerializer::Deserialize(Reader, RootJsonObject) || !RootJsonObject.IsValid())
    {
        return false;
    }

    TSharedPtr<FJsonValue> InnerJsonValue = RootJsonObject->TryGetField(TEXT("value"));
    
    if (!InnerJsonValue.IsValid() || InnerJsonValue->IsNull())
    {
        return false;
    }

    return FJsonObjectConverter::JsonValueToUProperty(
        InnerJsonValue,
        Property,
        Property->ContainerPtrToValuePtr<void>(Object),
        0,
        0
    );
}


// ------------------------------------------------------------
// Read choices
// ------------------------------------------------------------

static void GetChoices(
    UClass* MissionClass,
    const FProperty* SettingProperty,
    TArray<FString>& OutChoices)
{
    OutChoices.Empty();

    const FString SettingName =
        SettingProperty->GetName();

    const FString ChoicesName =
        MissionSettingsConstants::ChoicePrefix +
        SettingName.RightChop(
            MissionSettingsConstants::SettingPrefix.Len()
        );

    FProperty* ChoicesProperty =
        MissionClass->FindPropertyByName(
            FName(*ChoicesName)
        );

    if (!ChoicesProperty)
    {
        return;
    }

    FArrayProperty* ArrayProperty =
        CastField<FArrayProperty>(ChoicesProperty);

    if (!ArrayProperty)
    {
        return;
    }

    UObject* CDO = MissionClass->GetDefaultObject();

    if (!CDO)
    {
        return;
    }

    FScriptArrayHelper ArrayHelper(
        ArrayProperty,
        ArrayProperty->ContainerPtrToValuePtr<void>(CDO)
    );

    for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
    {
        void* Element =
            ArrayHelper.GetRawPtr(Index);

        FProperty* InnerProperty =
            ArrayProperty->Inner;

        if (FIntProperty* IntProperty =
            CastField<FIntProperty>(InnerProperty))
        {
            const int32 Value =
                IntProperty->GetPropertyValue(Element);

            OutChoices.Add(LexToString(Value));
        }
        else if (FDoubleProperty* FloatProperty =
            CastField<FDoubleProperty>(InnerProperty))
        {
            const double Value =
                FloatProperty->GetPropertyValue(Element);

            OutChoices.Add(LexToString(Value));
        }
    }
}


// ------------------------------------------------------------
// Get definitions
// ------------------------------------------------------------

TArray<FMissionSettingDefinition>
UECRMissionSettingsBlueprintLibrary::GetMissionSettingDefinitions(
    UClass* MissionClass)
{
    TArray<FMissionSettingDefinition> Result;

    if (!MissionClass)
    {
        return Result;
    }

    UObject* CDO =
        MissionClass->GetDefaultObject();

    for (TFieldIterator<FProperty> It(MissionClass);
         It;
         ++It)
    {
        FProperty* Property = *It;

        if (!IsSettingProperty(Property))
        {
            continue;
        }
        
        // if (!IsSupportedSettingType(Property))
        // {
        //     continue;
        // }

        FMissionSettingDefinition Definition;

        Definition.PropertyName =
            Property->GetFName();

        Definition.DisplayName =
            FText::FromString(
                GetDisplayName(Property)
            );

        Definition.Type =
            GetSettingType(Property);

        GetChoices(
            MissionClass,
            Property,
            Definition.Choices
        );

        ReadPropertyValue(
            CDO,
            Property,
            Definition.DefaultValue
        );

        Result.Add(Definition);
    }

    return Result;
}


// ------------------------------------------------------------
// Get value from instance
// ------------------------------------------------------------

bool UECRMissionSettingsBlueprintLibrary::GetMissionSettingValue(
    UObject* MissionInstance,
    FName PropertyName,
    FString& OutValue)
{
    OutValue.Empty();

    if (!MissionInstance)
    {
        return false;
    }

    FProperty* Property =
        MissionInstance->GetClass()->FindPropertyByName(
            PropertyName
        );

    if (!Property)
    {
        return false;
    }

    return ReadPropertyValue(
        MissionInstance,
        Property,
        OutValue
    );
}


// ------------------------------------------------------------
// Apply stored settings
// ------------------------------------------------------------

bool UECRMissionSettingsBlueprintLibrary::ApplyMissionSettings(
    UObject* MissionInstance,
    const FMissionSettings& Settings)
{
    if (!MissionInstance)
    {
        return false;
    }

    bool bAllSucceeded = true;

    for (const FMissionSettingValue& Setting :
         Settings.Values)
    {
        FProperty* Property =
            MissionInstance->GetClass()->FindPropertyByName(
                Setting.PropertyName
            );

        if (!Property)
        {
            // Setting may belong to another mission type.
            continue;
        }

        if (!IsSettingProperty(Property))
        {
            continue;
        }

        if (!WritePropertyValue(
            MissionInstance,
            Property,
            Setting.Value))
        {
            bAllSucceeded = false;
        }
    }

    return bAllSucceeded;
}


// ------------------------------------------------------------
// Store/update value
// ------------------------------------------------------------

void UECRMissionSettingsBlueprintLibrary::SetStoredMissionSetting(
    FMissionSettings& Settings,
    FName PropertyName,
    FString Value)
{
    for (FMissionSettingValue& Existing :
         Settings.Values)
    {
        if (Existing.PropertyName == PropertyName)
        {
            Existing.Value = MoveTemp(Value);
            return;
        }
    }

    FMissionSettingValue NewSetting;

    NewSetting.PropertyName = PropertyName;
    NewSetting.Value = MoveTemp(Value);

    Settings.Values.Add(MoveTemp(NewSetting));
}