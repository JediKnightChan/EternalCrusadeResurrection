// Copyright Epic Games, Inc. All Rights Reserved.

#include "WindowsEOSHelpers.h"

#if WITH_EOS_SDK

#include "EOSShared.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#if WITH_EOS_RTC
#include "Windows/eos_Windows.h"
#endif

IEOSPlatformHandlePtr FWindowsEOSHelpers::CreatePlatform(EOS_Platform_Options& PlatformOptions)
{
#if WITH_EOS_RTC
	const FTCHARToUTF8 Utf8XAudioPath(*IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Windows/XAudio2_9"), PLATFORM_64BITS ? TEXT("x64") : TEXT("x86"), TEXT("xaudio2_9redist.dll"))));

	EOS_Windows_RTCOptions WindowsRtcOptions = { 0 };
	WindowsRtcOptions.ApiVersion = EOS_WINDOWS_RTCOPTIONS_API_LATEST;
	static_assert(EOS_WINDOWS_RTCOPTIONS_API_LATEST == 1, "EOS_Windows_RTCOptions updated, check new fields");
	WindowsRtcOptions.XAudio29DllPath = Utf8XAudioPath.Get();

	EOS_Platform_RTCOptions* RTCOptions = const_cast<EOS_Platform_RTCOptions*>(PlatformOptions.RTCOptions);
	if (ensure(RTCOptions))
	{
		RTCOptions->PlatformSpecificOptions = &WindowsRtcOptions;
	}
#endif // WITH_EOS_RTC

	return FEOSHelpers::CreatePlatform(PlatformOptions);
}

#endif // WITH_EOS_SDK