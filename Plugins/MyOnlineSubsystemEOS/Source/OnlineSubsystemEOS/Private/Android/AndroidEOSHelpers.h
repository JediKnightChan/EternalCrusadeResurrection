// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EOS_SDK

#include "EOSHelpers.h"

class FAndroidEOSHelpers : public FEOSHelpers
{
public:
	virtual void PlatformTriggerLoginUI(FOnlineSubsystemEOS* EOSSubsystem, const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate) override;
};

using FPlatformEOSHelpers = FAndroidEOSHelpers;

#endif