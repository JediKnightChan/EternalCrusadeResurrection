// Copyleft: All rights reversed


#include "ECRGameInstance.h"

#include <eos_auth_types.h>

#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Engine/World.h"


#define LOCTEXT_NAMESPACE "GameInstanceNamespace"

const FName GSessionName = FName{"USER_CREATED_MATCH"};


UECRGameInstance::UECRGameInstance()
{
	bIsLoggedIn = false;
}


void UECRGameInstance::Init()
{
	Super::Init();
	OnlineSubsystem = IOnlineSubsystem::Get();
}

void UECRGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UECRGameInstance::LoginViaEpic()
{
	// ReSharper disable once StringLiteralTypo
	Login("accountportal");
}

void UECRGameInstance::LoginViaDevice()
{
	// ReSharper disable once StringLiteralTypo
	Login("persistentauth");
}


void UECRGameInstance::Login(FString LoginType)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			FOnlineAccountCredentials OnlineAccountCredentials;
			OnlineAccountCredentials.Id = "";
			OnlineAccountCredentials.Token = "";
			OnlineAccountCredentials.Type = LoginType;

			OnlineIdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UECRGameInstance::OnLoginComplete);
			OnlineIdentityPtr->Login(0, OnlineAccountCredentials);
		}
	}
}


void UECRGameInstance::OnLoginComplete(int32 LocalUserNum, const bool bWasSuccessful, const FUniqueNetId& UserId,
                                       const FString& Error)
{
	bIsLoggedIn = bWasSuccessful;

	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr OnlineIdentityPtr = OnlineSubsystem->GetIdentityInterface())
		{
			OnlineIdentityPtr->ClearOnLoginCompleteDelegates(0, this);
		}
	}

	const FString SuccessLogin = bWasSuccessful ? "true" : "false";

	UE_LOG(LogTemp, Warning, TEXT("Login successful: %s %s"), *(SuccessLogin), *(Error));

	if (bWasSuccessful)
	{
		ShowMainMenu(true);
	}
	else
	{
		ShowErrorMessage(LOCTEXT("LoginBWasSuccessfulFalse", "Login failed"));
	}
}


void UECRGameInstance::CreateMatch(const int32 NumPublicConnections, const bool bIsDedicated)
{
	// Check if logged in
	if (!bIsLoggedIn)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attempt to create match, but not logged in!"));
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			FOnlineSessionSettings SessionSettings;
			SessionSettings.NumPublicConnections = NumPublicConnections;
			SessionSettings.bIsDedicated = bIsDedicated;
			SessionSettings.bIsLANMatch = false;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bAllowJoinInProgress = true;
			SessionSettings.bAllowJoinViaPresence = false;
			SessionSettings.bUsesPresence = true;

			OnlineSessionPtr->OnCreateSessionCompleteDelegates.AddUObject(
				this, &UECRGameInstance::OnCreateMatchComplete);
			OnlineSessionPtr->CreateSession(0, GSessionName, SessionSettings);
		}
	}
}


void UECRGameInstance::OnCreateMatchComplete(FName SessionName, const bool bWasSuccessful)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr OnlineSessionPtr = OnlineSubsystem->GetSessionInterface())
		{
			OnlineSessionPtr->ClearOnCreateSessionCompleteDelegates(this);
		}
	}

	if (bWasSuccessful)
	{
		// Display loading screen as loading map
		ShowLoadingScreen(LoadingMap);
		// Load map with listen parameter
		GetWorld()->ServerTravel("ThirdPersonMap?listen");
	}
	else
	{
		ShowErrorMessage(LOCTEXT("MatchCreationBWasSuccessfulFalse", "Couldn't create match"));
	}
}

#undef LOCTEXT_NAMESPACE
