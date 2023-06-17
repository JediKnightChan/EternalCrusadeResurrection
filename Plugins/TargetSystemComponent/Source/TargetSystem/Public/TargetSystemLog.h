// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

TARGETSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogTargetSystem, Display, All);

#define TS_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogTargetSystem, Verbosity, Format, ##__VA_ARGS__); \
}
