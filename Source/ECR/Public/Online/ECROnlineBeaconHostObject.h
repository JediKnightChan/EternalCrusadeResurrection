#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "ECROnlineBeaconHostObject.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClientLeft, FUniqueNetIdRepl, UniqueNetId);


UCLASS(Blueprintable, BlueprintType, Transient, NotPlaceable, Config = Engine)
class AECROnlineBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_UCLASS_BODY()
	//~ Begin AOnlineBeaconHost Interface
	/** You can do stuff in this one if you want, but we just use the super for this example */
	virtual AOnlineBeaconClient* SpawnBeaconActor(class UNetConnection* ClientConnection) override;
	virtual void OnClientConnected(class AOnlineBeaconClient* NewClientActor,
	                               class UNetConnection* ClientConnection) override;
	virtual void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	//~ End AOnlineBeaconHost Interface

	/** In case you ever want to do other things */
	virtual bool Init();

	/** Client received update from client */
	UPROPERTY(BlueprintAssignable, Category = "ECRBeacon|Server")
	FOnClientLeft OnClientLeft;
};
