// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Settings/ECRSettingsShared.h"

#include "ECRAimSensitivityData.generated.h"

/** Defines a set of gamepad sensitivity to a float value. */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "ECR Aim Sensitivity Data", ShortTooltip = "Data asset used to define a map of Gamepad Sensitivty to a float value."))
class ECR_API UECRAimSensitivityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UECRAimSensitivityData(const FObjectInitializer& ObjectInitializer);
	
	const float SensitivityEnumToFloat(const EECRGamepadSensitivity InSensitivity) const;
	
protected:
	/** Map of SensitivityMap settings to their corresponding float */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<EECRGamepadSensitivity, float> SensitivityMap;
};