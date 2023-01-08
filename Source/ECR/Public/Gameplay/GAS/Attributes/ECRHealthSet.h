// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "ECRAttributeSet.h"
#include "NativeGameplayTags.h"
#include "ECRHealthSet.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Damage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageImmunity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageSelfDestruct);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_FellOutOfWorld);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ECR_Damage_Message);


/**
 * Basic class for taking damage.
 * Characters take damage via ECRCharacterHealthSet, which extends this class.
 */
UCLASS(Abstract)
class ECR_API UECRHealthSet : public UECRAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Health,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Health;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxHealth,
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData MaxHealth;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below) 
	// -------------------------------------------------------------------

	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category="ECR|Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health or other attributes (eg shield)
	UPROPERTY(BlueprintReadOnly, Category="ECR|Health", Meta=(HideFromModifiers, AllowPrivateAccess=true))
	FGameplayAttributeData Damage;

protected:
	/** Send message about damage to other subsystems */
	void SendDamageMessage(const FGameplayEffectModCallbackData& DamageData) const;

	/** Return if character is ready ti die */
	virtual bool GetIsReadyToDie() const;

	/** Checks if ready to die and broadcast event */
	void CheckIfReadyToDie(const FGameplayEffectModCallbackData& Data);

	/** Check for damage immunity in PreGameplayEffectExecute */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;

	/** Broadcast out of health event, apply healing in PostGameplayEffectExecute */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** Clamp attribute base value in PreAttributeBaseChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** React to MaxHealth change by clamping Health in PostAttributeChange */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/** Clamp attributes Health [0, MaxHealth] and MaxHealth [1, inf] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// RepNotifies

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;

protected:
	// Used to track when the health reaches 0 to trigger ReadyToDie event only once
	bool bReadyToDie;

public:
	UECRHealthSet();

	// Attribute accessors
	ATTRIBUTE_ACCESSORS(UECRHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UECRHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UECRHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UECRHealthSet, Damage);

	// Delegate to broadcast when the health attribute reaches zero.
	mutable FECRAttributeEvent OnReadyToDie;
};
