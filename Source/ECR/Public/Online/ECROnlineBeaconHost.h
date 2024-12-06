#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconHost.h"
#include "ECROnlineBeaconHost.generated.h"

class AOnlineBeaconHostObject;

UCLASS(Blueprintable, BlueprintType, Transient, NotPlaceable, Config=Engine)
class AECROnlineBeaconHost : public AOnlineBeaconHost
{
	GENERATED_BODY()

	AECROnlineBeaconHost(const FObjectInitializer& ObjectInitializer);

public:
	/** Blueprint accessor to init the beacon host */
	UFUNCTION(BlueprintCallable, Category = "PingBeaconHost")
	bool Start();

	/** A blueprint helper to add our PingBeaconHostObject */
	UFUNCTION(BlueprintCallable, Category = "PingBeaconHost")
	void AddHost(AOnlineBeaconHostObject* HostObject);

protected:
	/** If we successfully started are not */
	bool IsReady;
protected:
	virtual bool InitBase() override;
};
