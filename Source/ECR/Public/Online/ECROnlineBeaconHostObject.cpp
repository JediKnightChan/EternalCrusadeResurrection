#include "ECROnlineBeaconHostObject.h"
#include "ECROnlineBeacon.h"

AECROnlineBeaconHostObject::AECROnlineBeaconHostObject(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	/** Set our actual actor client class this host object will handle */
	ClientBeaconActorClass = AECROnlineBeacon::StaticClass();
	/** Set the beacon type name **/
	BeaconTypeName = ClientBeaconActorClass->GetName();

	/** Make sure we can tick **/
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

bool AECROnlineBeaconHostObject::Init()
{
	//just returning true for now
	return true;
}

void AECROnlineBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor,
                                                   UNetConnection* ClientConnection)
{
	//Call super
	Super::OnClientConnected(NewClientActor, ClientConnection);

	//Cast to our actual APingBeacon
	if (AECROnlineBeacon* BeaconClient = Cast<AECROnlineBeacon>(NewClientActor))
	{
		//It's good, so lets rpc back to the client and tell it we are ready
		BeaconClient->Ready();
		BeaconClient->GetNetConnection()->PlayerId;
	}
}

void AECROnlineBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	Super::NotifyClientDisconnected(LeavingClientActor);

	if (AECROnlineBeacon* BeaconClient = Cast<AECROnlineBeacon>(LeavingClientActor))
	{
		//It's good, so lets rpc back to the client and tell it we are ready
		OnClientLeft.Broadcast(BeaconClient->GetOwningPlayerId());
	}
}

AOnlineBeaconClient* AECROnlineBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	//Just super for now, technically you can return NULL here as well to prevent spawning
	return Super::SpawnBeaconActor(ClientConnection);
}
