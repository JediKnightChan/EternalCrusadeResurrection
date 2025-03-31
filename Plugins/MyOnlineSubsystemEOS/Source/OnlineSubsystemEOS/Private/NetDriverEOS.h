// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NetDriverEOSBase.h"
#include "NetDriverEOS.generated.h"

class ISocketSubsystem;

UCLASS(Transient, Config=Engine)
class ONLINESUBSYSTEMEOS_API UNetDriverEOS
	: public UNetDriverEOSBase
{
	GENERATED_BODY()

public:
};