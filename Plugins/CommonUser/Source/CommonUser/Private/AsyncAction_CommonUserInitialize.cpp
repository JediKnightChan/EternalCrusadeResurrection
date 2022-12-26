// Copyright Epic Games, Inc. All Rights Reserved.

#include "AsyncAction_CommonUserInitialize.h"
#include "TimerManager.h"

UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::InitializeForLocalPlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex, int32 ControllerId, bool bCanUseGuestLogin)
{
	UAsyncAction_CommonUserInitialize* Action = NewObject<UAsyncAction_CommonUserInitialize>();

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		
		Action->Params.RequestedPrivilege = ECommonUserPrivilege::CanPlay;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.ControllerId = ControllerId;
		Action->Params.bCanUseGuestLogin = bCanUseGuestLogin;
		Action->Params.bCanCreateNewLocalPlayer = true;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

UAsyncAction_CommonUserInitialize* UAsyncAction_CommonUserInitialize::LoginForOnlinePlay(UCommonUserSubsystem* Target, int32 LocalPlayerIndex)
{
	UAsyncAction_CommonUserInitialize* Action = NewObject<UAsyncAction_CommonUserInitialize>();

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		
		Action->Params.RequestedPrivilege = ECommonUserPrivilege::CanPlayOnline;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.bCanCreateNewLocalPlayer = false;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

void UAsyncAction_CommonUserInitialize::HandleFailure()
{
	const UCommonUserInfo* UserInfo = nullptr;
	if (Subsystem.IsValid())
	{
		UserInfo = Subsystem->GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex);
	}
	HandleInitializationComplete(UserInfo, false, NSLOCTEXT("CommonUser", "LoginFailedEarly", "Unable to start login process"), Params.RequestedPrivilege, Params.OnlineContext);
}

void UAsyncAction_CommonUserInitialize::HandleInitializationComplete(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	if (ShouldBroadcastDelegates())
	{
		OnInitializationComplete.Broadcast(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);
	}

	SetReadyToDestroy();
}

void UAsyncAction_CommonUserInitialize::Activate()
{
	if (Subsystem.IsValid())
	{
		Params.OnUserInitializeComplete.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAsyncAction_CommonUserInitialize, HandleInitializationComplete));
		bool bSuccess = Subsystem->TryToInitializeUser(Params);

		if (!bSuccess)
		{
			// Call failure next frame
			FTimerManager* TimerManager = GetTimerManager();
			
			if (TimerManager)
			{
				TimerManager->SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UAsyncAction_CommonUserInitialize::HandleFailure));
			}
		}
	}
	else
	{
		SetReadyToDestroy();
	}	
}
