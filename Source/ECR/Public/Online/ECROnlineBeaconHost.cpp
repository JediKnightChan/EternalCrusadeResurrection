#include "ECROnlineBeaconHost.h"
#include "OnlineBeaconHostObject.h"

AECROnlineBeaconHost::AECROnlineBeaconHost(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	//Set the beacon host state to allow requests
	BeaconState = EBeaconState::AllowRequests;
}

bool AECROnlineBeaconHost::Start()
{
	//Call our init to start up the network interface
	IsReady = InitHost();

	return IsReady;
}

void AECROnlineBeaconHost::AddHost(AOnlineBeaconHostObject* HostObject)
{
	/** Make sure we inited properly */
	if (IsReady)
	{
		RegisterHost(HostObject);
	}
}

bool AECROnlineBeaconHost::InitBase()
{
	static const FName NAME_BeaconName(TEXT("BeaconSession"));

	GEngine->CreateNamedNetDriver(GetWorld(), NAME_BeaconName, NetDriverDefinitionName);

	UNetDriver* DriverSearchResult = GEngine->FindNamedNetDriver(GetWorld(), NAME_BeaconName);
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
