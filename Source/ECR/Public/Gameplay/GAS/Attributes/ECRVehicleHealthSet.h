// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ECRHealthSet.h"
#include "ECRVehicleHealthSet.generated.h"

/**
 * Vehicle defensive attributes set
 */
UCLASS()
class ECR_API UECRVehicleHealthSet : public UECRAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Engine_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Engine_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Transmission_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Transmission_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_CannonBreech_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData CannonBreech_Health;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_CannonBarrel_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData CannonBarrel_Health;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Radiator_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Radiator_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_FuelTank_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData FuelTank_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Track1_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Track1_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Track2_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Track2_Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_AmmoRack_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData AmmoRack_Health;

	// Array of health attributes above
	TArray<FGameplayAttribute> HealthAttributes;
protected:
	/** Check for damage immunity for shield in PreGameplayEffectExecute */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/** Clamp attributes [0, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// RepNotifies

	UFUNCTION()
	void OnRep_Engine_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Transmission_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CannonBreech_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_CannonBarrel_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Radiator_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_FuelTank_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Track1_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Track2_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_AmmoRack_Health(const FGameplayAttributeData& OldValue) const;
public:
	UECRVehicleHealthSet();

	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, Engine_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, Transmission_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, CannonBreech_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, CannonBarrel_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, Radiator_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, FuelTank_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, Track1_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, Track2_Health);
	ATTRIBUTE_ACCESSORS(UECRVehicleHealthSet, AmmoRack_Health);
};
