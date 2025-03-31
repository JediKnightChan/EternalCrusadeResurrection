// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineVoiceEOSPlus.h"
#include "OnlineSubsystemEOSPlus.h"
#include "OnlineError.h"

FOnlineVoiceEOSPlus::FOnlineVoiceEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	BaseVoiceInterface = EOSPlus->BaseOSS->GetVoiceInterface();
}

FOnlineVoiceEOSPlus::~FOnlineVoiceEOSPlus()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->ClearOnPlayerTalkingStateChangedDelegates(this);
	}
}

FUniqueNetIdEOSPlusPtr FOnlineVoiceEOSPlus::GetNetIdPlus(const FString& SourceId) const
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineVoiceEOSPlus::Initialize()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->AddOnPlayerTalkingStateChangedDelegate_Handle(FOnPlayerTalkingStateChangedDelegate::CreateThreadSafeSP(this, &FOnlineVoiceEOSPlus::OnPlayerTalkingStateChanged));
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::Initialize] BaseVoiceInterface delegates not bound. Base interface not valid"));
	}
}

//~ Begin IOnlineVoice Interface

IVoiceEnginePtr FOnlineVoiceEOSPlus::CreateVoiceEngine()
{
	IVoiceEnginePtr Result = nullptr;
	
	if (BaseVoiceInterface.IsValid())
	{
		Result = BaseVoiceInterface->CreateVoiceEngine();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::CreateVoiceEngine] Unable to call method in base interface. Base interface not valid"));
	}
	
	return Result;
}

void FOnlineVoiceEOSPlus::ProcessMuteChangeNotification()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->ProcessMuteChangeNotification();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::ProcessMuteChangeNotification] Unable to call method in base interface. Base interface not valid"));
	}
}

void FOnlineVoiceEOSPlus::StartNetworkedVoice(uint8 LocalUserNum)
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->StartNetworkedVoice(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::StartNetworkedVoice] Unable to call method in base interface. Base interface not valid"));
	}
}

void FOnlineVoiceEOSPlus::StopNetworkedVoice(uint8 LocalUserNum)
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->StopNetworkedVoice(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::StopNetworkedVoice] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineVoiceEOSPlus::RegisterLocalTalker(uint32 LocalUserNum)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		bResult = BaseVoiceInterface->RegisterLocalTalker(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::RegisterLocalTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineVoiceEOSPlus::RegisterLocalTalkers()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->RegisterLocalTalkers();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::RegisterLocalTalkers] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineVoiceEOSPlus::UnregisterLocalTalker(uint32 LocalUserNum)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		bResult = BaseVoiceInterface->UnregisterLocalTalker(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnregisterLocalTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineVoiceEOSPlus::UnregisterLocalTalkers()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->UnregisterLocalTalkers();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnregisterLocalTalkers] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineVoiceEOSPlus::RegisterRemoteTalker(const FUniqueNetId& UniqueId)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UniqueId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->RegisterRemoteTalker(*NetIdPlus->GetBaseNetId());
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::RegisterRemoteTalker] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *UniqueId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::RegisterRemoteTalker] Unable to call method in base interface. Unknown user (%s)"), *UniqueId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::RegisterRemoteTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::UnregisterRemoteTalker(const FUniqueNetId& UniqueId)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UniqueId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->UnregisterRemoteTalker(*NetIdPlus->GetBaseNetId());
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnregisterRemoteTalker] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *UniqueId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnregisterRemoteTalker] Unable to call method in base interface. Unknown user (%s)"), *UniqueId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnregisterRemoteTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

void FOnlineVoiceEOSPlus::RemoveAllRemoteTalkers()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->RemoveAllRemoteTalkers();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::RemoveAllRemoteTalkers] Unable to call method in base interface. Base interface not valid"));
	}
}

bool FOnlineVoiceEOSPlus::IsHeadsetPresent(uint32 LocalUserNum)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		bResult = BaseVoiceInterface->IsHeadsetPresent(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsHeadsetPresent] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::IsLocalPlayerTalking(uint32 LocalUserNum)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		bResult = BaseVoiceInterface->IsLocalPlayerTalking(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsLocalPlayerTalking] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::IsRemotePlayerTalking(const FUniqueNetId& UniqueId)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UniqueId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->IsRemotePlayerTalking(*NetIdPlus->GetBaseNetId());
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsRemotePlayerTalking] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *UniqueId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsRemotePlayerTalking] Unable to call method in base interface. Unknown user (%s)"), *UniqueId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsRemotePlayerTalking] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UniqueId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->IsMuted(LocalUserNum, *NetIdPlus->GetBaseNetId());
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsMuted] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *UniqueId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsMuted] Unable to call method in base interface. Unknown user (%s)"), *UniqueId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::IsMuted] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->MuteRemoteTalker(LocalUserNum, *NetIdPlus->GetBaseNetId(), bIsSystemWide);
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::MuteRemoteTalker] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *PlayerId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::MuteRemoteTalker] Unable to call method in base interface. Unknown user (%s)"), *PlayerId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::MuteRemoteTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

bool FOnlineVoiceEOSPlus::UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide)
{
	bool bResult = false;

	if (BaseVoiceInterface.IsValid())
	{
		FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(PlayerId.ToString());
		if (NetIdPlus.IsValid())
		{
			if (ensure(NetIdPlus->GetBaseNetId().IsValid()))
			{
				bResult = BaseVoiceInterface->UnmuteRemoteTalker(LocalUserNum, *NetIdPlus->GetBaseNetId(), bIsSystemWide);
			}
			else
			{
				UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnmuteRemoteTalker] Unable to call method in base interface. BaseNetId not valid for user (%s)."), *PlayerId.ToString());
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnmuteRemoteTalker] Unable to call method in base interface. Unknown user (%s)"), *PlayerId.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::UnmuteRemoteTalker] Unable to call method in base interface. Base interface not valid"));
	}

	return bResult;
}

TSharedPtr<FVoicePacket> FOnlineVoiceEOSPlus::SerializeRemotePacket(FArchive& Ar)
{
	TSharedPtr<FVoicePacket> Result = nullptr;
	
	if (BaseVoiceInterface.IsValid())
	{
		Result = BaseVoiceInterface->SerializeRemotePacket(Ar);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::SerializeRemotePacket] Unable to call method in base interface. Base interface not valid"));
	}
	
	return Result;
}

TSharedPtr<FVoicePacket> FOnlineVoiceEOSPlus::GetLocalPacket(uint32 LocalUserNum)
{
	TSharedPtr<FVoicePacket> Result = nullptr;
	
	if (BaseVoiceInterface.IsValid())
	{
		Result = BaseVoiceInterface->GetLocalPacket(LocalUserNum);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::GetLocalPacket] Unable to call method in base interface. Base interface not valid"));
	}
	
	return Result;
}

int32 FOnlineVoiceEOSPlus::GetNumLocalTalkers()
{
	int32 Result = 0;

	if (BaseVoiceInterface.IsValid())
	{
		Result = BaseVoiceInterface->GetNumLocalTalkers();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::GetNumLocalTalkers] Unable to call method in base interface. Base interface not valid"));
	}

	return Result;
}

void FOnlineVoiceEOSPlus::ClearVoicePackets()
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->ClearVoicePackets();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::ClearVoicePackets] Unable to call method in base interface. Base interface not valid"));
	}
}

void FOnlineVoiceEOSPlus::Tick(float DeltaTime)
{
	if (BaseVoiceInterface.IsValid())
	{
		BaseVoiceInterface->Tick(DeltaTime);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::Tick] Unable to call method in base interface. Base interface not valid"));
	}
}

FString FOnlineVoiceEOSPlus::GetVoiceDebugState() const
{
	FString Result;

	if (BaseVoiceInterface.IsValid())
	{
		Result = BaseVoiceInterface->GetVoiceDebugState();
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineVoiceEOSPlus::GetVoiceDebugState] Unable to call method in base interface. Base interface not valid"));
	}

	return Result;
}
	
void FOnlineVoiceEOSPlus::OnPlayerTalkingStateChanged(FUniqueNetIdRef TalkerId, bool bIsTalking)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(TalkerId->ToString());
	if (NetIdPlus.IsValid())
	{
		// We don't want to trigger it anyways if the NetIdPlus is not valid like in other interfaces, because this one takes a FUniqueNetIdRef as parameter
		TriggerOnPlayerTalkingStateChangedDelegates(NetIdPlus.ToSharedRef(), bIsTalking);
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineVoiceEOSPlus::OnPlayerTalkingStateChanged] Unknown user (%s)"), *TalkerId->ToString());
	}	
}

//~ End IOnlineVoice Interface