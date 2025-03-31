// Copyright Epic Games, Inc. All Rights Reserved.

#include "AndroidEOSHelpers.h"

#if WITH_EOS_SDK

void FAndroidEOSHelpers::PlatformTriggerLoginUI(FOnlineSubsystemEOS* InEOSSubsystem, const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	ShowAccountPortalUI(InEOSSubsystem, ControllerIndex, Delegate);
}

#endif
