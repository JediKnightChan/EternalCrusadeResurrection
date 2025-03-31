// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "OnlineSubsystem.h"
#include "Interfaces/VoiceInterface.h"
#include "OnlineUserEOSPlus.h"

class FOnlineSubsystemEOSPlus;

/**
 * Interface for encapsulating the platform voice interface
 */
class FOnlineVoiceEOSPlus :
	public IOnlineVoice,
	public TSharedFromThis<FOnlineVoiceEOSPlus, ESPMode::ThreadSafe>
{
public:
	FOnlineVoiceEOSPlus() = delete;
	virtual ~FOnlineVoiceEOSPlus();

	void Initialize();

	//~ Begin IOnlineVoice Interface
	virtual void StartNetworkedVoice(uint8 LocalUserNum) override;
	virtual void StopNetworkedVoice(uint8 LocalUserNum) override;
	virtual bool RegisterLocalTalker(uint32 LocalUserNum) override;
	virtual void RegisterLocalTalkers() override;
	virtual bool UnregisterLocalTalker(uint32 LocalUserNum) override;
	virtual void UnregisterLocalTalkers() override;
	virtual bool RegisterRemoteTalker(const FUniqueNetId& UniqueId) override;
	virtual bool UnregisterRemoteTalker(const FUniqueNetId& UniqueId) override;
	virtual void RemoveAllRemoteTalkers() override;
	virtual bool IsHeadsetPresent(uint32 LocalUserNum) override;
	virtual bool IsLocalPlayerTalking(uint32 LocalUserNum) override;
	virtual bool IsRemotePlayerTalking(const FUniqueNetId& UniqueId) override;
	virtual bool IsMuted(uint32 LocalUserNum, const FUniqueNetId& UniqueId) const override;
	virtual bool MuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	virtual bool UnmuteRemoteTalker(uint8 LocalUserNum, const FUniqueNetId& PlayerId, bool bIsSystemWide) override;
	virtual TSharedPtr<FVoicePacket> SerializeRemotePacket(FArchive& Ar) override;
	virtual TSharedPtr<FVoicePacket> GetLocalPacket(uint32 LocalUserNum) override;
	virtual int32 GetNumLocalTalkers() override;
	virtual void ClearVoicePackets() override;
	virtual void Tick(float DeltaTime) override;
	virtual FString GetVoiceDebugState() const override;
	//~ End IOnlineVoice Interface

PACKAGE_SCOPE:
	FOnlineVoiceEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem);

	virtual IVoiceEnginePtr CreateVoiceEngine() override;
	virtual void ProcessMuteChangeNotification() override;
	
	void OnPlayerTalkingStateChanged(FUniqueNetIdRef TalkerId, bool bIsTalking);

private:
	FUniqueNetIdEOSPlusPtr GetNetIdPlus(const FString& SourceId) const;

	/** Reference to the owning EOS plus subsystem */
	FOnlineSubsystemEOSPlus* EOSPlus;
	
	// We don't support EOS mirroring yet
	IOnlineVoicePtr BaseVoiceInterface;
};

typedef TSharedPtr<FOnlineVoiceEOSPlus, ESPMode::ThreadSafe> FOnlineVoiceEOSPlusPtr;
