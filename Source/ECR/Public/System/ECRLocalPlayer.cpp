// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRLocalPlayer.h"
#include "Settings/ECRSettingsLocal.h"
#include "AudioMixerBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

UECRLocalPlayer::UECRLocalPlayer()
{
}

void UECRLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (UECRSettingsLocal* LocalSettings = GetLocalSettings())
	{
		LocalSettings->OnAudioOutputDeviceChanged.AddUObject(this, &UECRLocalPlayer::OnAudioOutputDeviceChanged);
	}
}


UECRSettingsLocal* UECRLocalPlayer::GetLocalSettings() const
{
	return UECRSettingsLocal::Get();
}


void UECRLocalPlayer::OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId)
{
	FOnCompletedDeviceSwap DevicesSwappedCallback;
	DevicesSwappedCallback.BindUFunction(this, FName("OnCompletedAudioDeviceSwap"));
	UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(GetWorld(), InAudioOutputDeviceId, DevicesSwappedCallback);
}

void UECRLocalPlayer::OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult)
{
	if (SwapResult.Result == ESwapAudioOutputDeviceResultState::Failure)
	{
	}
}
