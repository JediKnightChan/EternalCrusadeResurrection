#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "ECROnlineBeaconClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeaconPingComplete, int32, TimeMS);

DECLARE_LOG_CATEGORY_EXTERN(FBeaconLog, Log, All);

/**
 * Simple ping client beacon class
 */
UCLASS(Blueprintable, BlueprintType, transient, notplaceable, config = Engine)
class AECROnlineBeacon : public AOnlineBeaconClient
{
	GENERATED_BODY()

	AECROnlineBeacon(const FObjectInitializer& ObjectInitializer);

public:
	//~ Begin AOnlineBeaconClient Interface
	virtual void OnFailure() override;
	//~ End AOnlineBeaconClient Interface

	/** Send a ping RPC to the client */
	UFUNCTION(Client, Reliable)
	virtual void ClientPing();

	/** Let's us know the beacon is ready so we can prep the initial start time for ping round trip */
	UFUNCTION(Client, Reliable)
	virtual void Ready();

	/** Send a pong RPC to the host */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerPong();

	/** Provide Blueprint Access to Start the Beacon **/
	UFUNCTION(BlueprintCallable, Category = "PingBeacon")
	bool Start(FString address, int32 port, const bool portOverride);

	/** Provide Blueprint access to disconnect and destroy the client beacon */
	UFUNCTION(BlueprintCallable, Category = "PingBeacon")
	void Disconnect();

public:
	/** Provide a Blueprint binding for the OnPingComplete event */
	UPROPERTY(BlueprintAssignable, Category = "PingBeacon")
	FOnBeaconPingComplete OnPingComplete;

	/** Need to add one also for a failure. That is up to you though! */

protected:
	/** Holds our initial start time in Ticks from FDateTime.Now().GetTicks() */
	int64 startTime;
protected:
	virtual bool InitBase() override;
};
