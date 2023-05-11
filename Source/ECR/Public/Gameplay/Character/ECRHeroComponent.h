#pragma once

#include "ECRPawnControlComponent.h"
#include "ECRHeroComponent.generated.h"

/**
 * UECRHeroComponent
 *
 * Pawn Control Component for characters
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class UECRHeroComponent : public UECRPawnControlComponent
{
	GENERATED_BODY()

public:
	UECRHeroComponent(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BindNativeActions(UECRInputComponent* ECRIC, const UECRInputConfig* InputConfig) override;

	virtual void Input_Move(const FInputActionValue& InputActionValue);
	virtual void Input_AutoRun(const FInputActionValue& InputActionValue);
};
