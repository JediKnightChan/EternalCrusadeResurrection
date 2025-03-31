#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "ECROnlineBeacon.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBeaconUpdateComplete, FString, JsonString, FUniqueNetIdRepl,
                                             UniqueNetId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBeaconChannelUpdateComplete, FString, Channel, FString, JsonString,
                                               FUniqueNetIdRepl, UniqueNetId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeaconFailure, FUniqueNetIdRepl, UniqueNetId);

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

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FUniqueNetIdRepl GetOwningPlayerId();

	/** Send an update RPC to the client */
	UFUNCTION(Client, Reliable)
	virtual void ClientPing(const FString& RepServerData);

	/** Let's us know the beacon is ready so we can prep the initial start time for ping round trip */
	UFUNCTION(Client, Reliable)
	virtual void Ready();

	/** Send a pong RPC to the host */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	virtual void ServerPong(const FString& Channel, const FString& ClientData);

	/** Provide Blueprint Access to Start the Beacon **/
	UFUNCTION(BlueprintCallable, Category = "ECRBeacon")
	bool Start(FString Address, int32 Port, const bool bOverridePort, const FUniqueNetIdRepl TargetPlayer);

	/** Provide Blueprint access to disconnect and destroy the client beacon */
	UFUNCTION(BlueprintCallable, Category = "ECRBeacon")
	void Disconnect();

	UFUNCTION(BlueprintAuthorityOnly, Category = "ECRBeacon|Server")
	void SetServerDataNoUpdate(FString NewServerData);

	UFUNCTION(BlueprintAuthorityOnly, Category = "ECRBeacon|Server")
	void SetServerDataAndUpdate(FString NewServerData);

	/** Client received update from server */
	UPROPERTY(BlueprintAssignable, Category = "ECRBeacon|Client")
	FOnBeaconUpdateComplete OnReceivedUpdateFromServer;

	/** Client received update from client */
	UPROPERTY(BlueprintAssignable, Category = "ECRBeacon|Server")
	FOnBeaconChannelUpdateComplete OnReceivedUpdateFromClient;

	UPROPERTY(BlueprintAssignable, Category = "ECRBeacon|Client")
	FOnBeaconFailure OnBeaconFailure;

protected:
	virtual bool InitBase() override;

private:
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess, ExposeOnSpawn))
	FString InitCallClientData;

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess, ExposeOnSpawn))
	FString InitCallChannel;

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess))
	FString ServerData;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess))
	FUniqueNetIdRepl CachedPlayerId;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess))
	bool bClientGotFirstServerData;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess, ExposeOnSpawn))
	FName DriverName;
};
