// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ECRHealthSet.h"
#include "ECRCharacterHealthSet.generated.h"

/**
 * Character defensive attributes set
 * (shield reacts to damage before health,
 * stamina reacts to consuming actions).
 */
UCLASS()
class ECR_API UECRCharacterHealthSet : public UECRHealthSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Shield,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Shield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxShield,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxShield;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_BleedingHealth,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData BleedingHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxBleedingHealth,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxBleedingHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_WalkSpeed,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData WalkSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Stamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Stamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_EvasionStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData EvasionStamina;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxEvasionStamina,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxEvasionStamina;

protected:
	/** Returns if ready to become wounded */
	virtual bool GetIsReadyToBecomeWounded() const;

	/** Returns if is ready to die */
	virtual bool GetIsReadyToDie() const override;

	/** Check if is ready to become wounded and broadcast event */
	void CheckIfReadyToBecomeWounded(const FGameplayEffectModCallbackData& Data);

	/** React to damage, healing */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxShield and MaxStamina change by clamping Shield and Stamina in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes Shield, Stamina [0, MaxShield/MaxStamina], MaxShield, MaxStamina [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

	// RepNotifies

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxShield(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_BleedingHealth(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxBleedingHealth(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_WalkSpeed(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_EvasionStamina(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxEvasionStamina(const FGameplayAttributeData& OldValue) const;

protected:
	// Used to track when the health reaches 0 to trigger ReadyToBecomeWounded event only once
	bool bReadyToBecomeWounded;

public:
	UECRCharacterHealthSet();

	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, Shield);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, MaxShield);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, BleedingHealth);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, MaxBleedingHealth);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, WalkSpeed);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, Stamina);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, MaxStamina);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, EvasionStamina);
	ATTRIBUTE_ACCESSORS(UECRCharacterHealthSet, MaxEvasionStamina);

	// Delegate to broadcast when the health attribute reaches zero.
	mutable FECRAttributeEvent OnReadyToBecomeWounded;
};
