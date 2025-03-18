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

void AECROnlineBeaconHostObject::UpdateServerData(FString NewData)
{
	ServerData = NewData;
	for (AECROnlineBeacon* Beacon : ConnectedClients)
	{
		if (Beacon)
		{
			Beacon->SetServerDataAndUpdate(NewData);
		}
	}
}

void AECROnlineBeaconHostObject::DisconnectClientBeacon(FUniqueNetIdRepl PlayerId)
{
	for (AECROnlineBeacon* Client : ConnectedClients)
	{
		if (Client->GetOwningPlayerId() == PlayerId)
		{
			DisconnectClient(Client);
		}
	}
}

void AECROnlineBeaconHostObject::OnReceivedUpdateFromClient(FString Channel, FString JsonString, FUniqueNetIdRepl UniqueNetId)
{
	OnReceivedUpdateFromClient_BP.Broadcast(Channel, JsonString, UniqueNetId);
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
		BeaconClient->SetServerDataNoUpdate(ServerData);
		BeaconClient->OnReceivedUpdateFromClient.AddDynamic(
			this, &AECROnlineBeaconHostObject::OnReceivedUpdateFromClient);
		BeaconClient->Ready();
		ConnectedClients.Add(BeaconClient);
	}
}

void AECROnlineBeaconHostObject::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	Super::NotifyClientDisconnected(LeavingClientActor);

	if (AECROnlineBeacon* BeaconClient = Cast<AECROnlineBeacon>(LeavingClientActor))
	{
		//It's good, so lets rpc back to the client and tell it we are ready
		OnClientLeft_BP.Broadcast(BeaconClient->GetOwningPlayerId());
		ConnectedClients.Remove(BeaconClient);
	}
}

AOnlineBeaconClient* AECROnlineBeaconHostObject::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	//Just super for now, technically you can return NULL here as well to prevent spawning
	return Super::SpawnBeaconActor(ClientConnection);
}
