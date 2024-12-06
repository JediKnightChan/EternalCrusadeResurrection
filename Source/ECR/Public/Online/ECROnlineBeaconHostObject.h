#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHostObject.h"
#include "ECROnlineBeaconHostObject.generated.h"


UCLASS(Blueprintable, BlueprintType, Transient, NotPlaceable, Config = Engine)
class AECROnlineBeaconHostObject : public AOnlineBeaconHostObject
{
	GENERATED_UCLASS_BODY()
	
	//~ Begin AOnlineBeaconHost Interface
	/** You can do stuff in this one if you want, but we just use the super for this example */
	virtual AOnlineBeaconClient* SpawnBeaconActor(class UNetConnection* ClientConnection) override;
	virtual void OnClientConnected(class AOnlineBeaconClient* NewClientActor,
	                               class UNetConnection* ClientConnection) override;
	//~ End AOnlineBeaconHost Interface

	/** In case you ever want to do other things */
	virtual bool Init();
};
