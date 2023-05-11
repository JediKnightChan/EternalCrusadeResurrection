#pragma once

#include "ECRPawnControlComponent.h"
#include "ECRVehicleComponent.generated.h"

UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class UECRVehicleComponent : public UECRPawnControlComponent
{
	GENERATED_BODY()

public:
	UECRVehicleComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnPawnReadyToInitialize() override;

	virtual void BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig) override;
};
