#include "ECROnlineBeacon.h"

DEFINE_LOG_CATEGORY(FBeaconLog);

AECROnlineBeacon::AECROnlineBeacon(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bClientGotFirstServerData = false;
	DriverName = FName{TEXT("BeaconSession")};
}

void AECROnlineBeacon::OnFailure()
{
	OnBeaconFailure.Broadcast(CachedPlayerId);
	Super::OnFailure();
}

FUniqueNetIdRepl AECROnlineBeacon::GetOwningPlayerId()
{
	FUniqueNetIdRepl PlayerId;
	if (UNetConnection* NetConnection = GetNetConnection())
	{
		PlayerId = NetConnection->PlayerId;
	}
	return PlayerId;
}

/** The rpc client ping implementation */
void AECROnlineBeacon::ClientPing_Implementation(const FString& RepServerData)
{
	OnReceivedUpdateFromServer.Broadcast(RepServerData, CachedPlayerId);
}

/** The rpc client ready implementation */
void AECROnlineBeacon::Ready_Implementation()
{
	//Call server pong rpc
	ServerPong(InitCallChannel, InitCallClientData);
}

bool AECROnlineBeacon::ServerPong_Validate(const FString& Channel, const FString& ClientData)
{
	return true;
}

/** ServerPong rpc implementation **/
void AECROnlineBeacon::ServerPong_Implementation(const FString& Channel, const FString& ClientData)
{
	OnReceivedUpdateFromClient.Broadcast(Channel, ClientData, GetOwningPlayerId());
	// Broadcast server data to client, if it's first communication
	if (!bClientGotFirstServerData)
	{
		ClientPing(ServerData);
		bClientGotFirstServerData = true;
	}
}

/** Our blueprint helper for stuff **/
bool AECROnlineBeacon::Start(FString Address, int32 Port, const bool bOverridePort, const FUniqueNetIdRepl TargetPlayer)
{
	CachedPlayerId = TargetPlayer;

	//Address must be an IP or valid domain name such as epicgames.com or 127.0.0.1
	//Do not include a port in the address! Beacons use a different port then the standard 7777 for connection
	FURL Url(nullptr, *Address, ETravelType::TRAVEL_Absolute);

	//overriding it with a user specified port?
	if (bOverridePort)
	{
		Url.Port = Port;
	}
	//if not overriding just pull the config for it based on the beacon host ListenPort
	else
	{
		int32 PortFromConfig;
		GConfig->GetInt(
			TEXT("/Script/OnlineSubsystemUtils.OnlineBeaconHost"), TEXT("ListenPort"), PortFromConfig, GEngineIni);
		Url.Port = PortFromConfig;
	}
	UE_LOG(LogTemp, Warning, TEXT("Url %s valid %d"), *Url.ToString(), Url.Valid ? 1:0)
	//Tell our beacon client to begin connection request to server address with our beacon port
	return InitClient(Url);
}

/** Our blueprint helper for disconnecting and destroying the beacon */
void AECROnlineBeacon::Disconnect()
{
	DestroyBeacon();
}

void AECROnlineBeacon::SetServerDataNoUpdate(FString NewServerData)
{
	ServerData = NewServerData;
}

void AECROnlineBeacon::SetServerDataAndUpdate(FString NewServerData)
{
	SetServerDataNoUpdate(NewServerData);
	ClientPing(NewServerData);
}

bool AECROnlineBeacon::InitBase()
{
	GEngine->CreateNamedNetDriver(GetWorld(), DriverName, NetDriverDefinitionName);

	UNetDriver* DriverSearchResult = GEngine->FindNamedNetDriver(GetWorld(), DriverName);
	if (DriverSearchResult)
	{
		NetDriver = DriverSearchResult;
	}

	if (NetDriver != nullptr)
	{
		if (NetDriver->GetWorld() == nullptr)
		{
			NetDriver->SetWorld(GetWorld());
		}

		HandleNetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &AOnlineBeacon::HandleNetworkFailure);
		SetNetDriverName(NetDriver->NetDriverName);
		return true;
	}

	return false;
}
