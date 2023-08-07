// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRReplaySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/DemoNetDriver.h"

UECRReplaySubsystem::UECRReplaySubsystem()
{
}

void UECRReplaySubsystem::PlayReplay(UECRReplayListEntry* Replay)
{
	if (Replay != nullptr)
	{
		FString DemoName = Replay->StreamInfo.Name;
		GetGameInstance()->PlayReplay(DemoName);
	}
}

void UECRReplaySubsystem::StartRecordingReplay(const FString Name, const FString FriendlyName)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->StartRecordingReplay(Name, FriendlyName);
	}
}

void UECRReplaySubsystem::StopRecordingReplay()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		GameInstance->StopRecordingReplay();
	}
}

void UECRReplaySubsystem::PausePlayingReplay()
{
	if (const UWorld* World = GetWorld())
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();
		check(WorldSettings != nullptr);

		if (WorldSettings->GetPauserPlayerState() == nullptr)
		{
			if (World->GetDemoNetDriver() != nullptr && World->GetDemoNetDriver()->ServerConnection != nullptr && World
				->
				GetDemoNetDriver()->ServerConnection->OwningActor != nullptr)
			{
				APlayerController* PlayerController = Cast<APlayerController>(
					World->GetDemoNetDriver()->ServerConnection->OwningActor);
				if (PlayerController != nullptr)
				{
					WorldSettings->SetPauserPlayerState(PlayerController->PlayerState);
				}
			}
		}
	}
}

void UECRReplaySubsystem::ResumePlayingReplay()
{
	if (const UWorld* World = GetWorld())
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();
		check(WorldSettings != nullptr);

		if (WorldSettings->GetPauserPlayerState())
		{
			WorldSettings->SetPauserPlayerState(nullptr);
		}
	}
}

void UECRReplaySubsystem::ChangeReplayPlayingSpeed(const float NewSpeed)
{
	if (UWorld* World = GetWorld())
	{
		AWorldSettings* WorldSettings = World->GetWorldSettings();
		check(WorldSettings != nullptr);

		WorldSettings->DemoPlayTimeDilation = NewSpeed;
	}
}

void UECRReplaySubsystem::SeekInActiveReplay(float TimeInSeconds)
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		DemoDriver->GotoTimeInSeconds(TimeInSeconds);
	}
}

float UECRReplaySubsystem::GetReplayLengthInSeconds() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoTotalTime();
	}
	return 0.0f;
}

float UECRReplaySubsystem::GetReplayCurrentTime() const
{
	if (UDemoNetDriver* DemoDriver = GetDemoDriver())
	{
		return DemoDriver->GetDemoCurrentTime();
	}
	return 0.0f;
}

UDemoNetDriver* UECRReplaySubsystem::GetDemoDriver() const
{
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		return World->GetDemoNetDriver();
	}
	return nullptr;
}
