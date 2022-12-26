// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

ECR_API DECLARE_LOG_CATEGORY_EXTERN(LogECR, Log, All);
ECR_API DECLARE_LOG_CATEGORY_EXTERN(LogECRExperience, Log, All);
ECR_API DECLARE_LOG_CATEGORY_EXTERN(LogECRAbilitySystem, Log, All);
ECR_API DECLARE_LOG_CATEGORY_EXTERN(LogECRTeams, Log, All);

ECR_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
