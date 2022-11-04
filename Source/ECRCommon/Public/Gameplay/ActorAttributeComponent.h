// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorAttributeComponent.generated.h"

// Player parameter (eg health) changed emitter: new current value and new max value
DECLARE_DELEGATE_TwoParams(FPlayerParameterChanged, float, float);

UCLASS(ClassGroup=(Gameplay), BlueprintType)
class ECRCOMMON_API UActorAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

	/** Current attribute (health / stamina / etc) value, can change with time */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentValue, VisibleAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess = "true"))
	float CurrentValue;

	/** Maximum attribute (health / stamina / etc) value,can change with time (buffs, etc) */
	UPROPERTY(ReplicatedUsing=OnRep_MaximumValue, VisibleAnywhere, BlueprintReadOnly,
		meta=(AllowPrivateAccess = "true"))
	float MaximumValue;

	/** RepNotify for changes made to current value */
	UFUNCTION()
	void OnRep_CurrentValue() const;

	/** RepNotify for changes made to maximum value */
	UFUNCTION()
	void OnRep_MaximumValue() const;

public:
	/** Sets default values for this component's properties */
	UActorAttributeComponent();

	/** Describes replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Delegate for processing changes of current and maximum value (eg on actor level) */
	FPlayerParameterChanged ProcessPlayerParameterChanged;

	/** Server - Reset current value to maximum value (at once restore health / stamina / etc) */
	UFUNCTION(BlueprintCallable)
	void ResetCurrentValueToMax();

	/** Server - Update max value */
	void SetMaxValue(float NewMaximumValue);

	/** Server - Lower current value by Damage */
	void ApplyDamage(float Damage);

	/** Get current value */
	FORCEINLINE float GetCurrentValue() const { return CurrentValue; }
};
