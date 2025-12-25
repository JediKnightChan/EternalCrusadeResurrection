// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRAttributeSet.h"
#include "ECRPveCombatSet.generated.h"


/**
 *  Defines attributes that are used only in PvE combat.
 */
UCLASS(BlueprintType)
class UECRPveCombatSet : public UECRAttributeSet
{
	GENERATED_BODY()

	// Main PvE resource (like mana, but mana name is already reserved by PvP sorcerers)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Resource, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Resource;

protected:
	/** Send message about damage to other subsystems */
	void SendResourceChangedMessage(const FGameplayEffectModCallbackData& ResourceData) const;
	
	/** Clamp attribute current value in PreAttributeChange */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Clamp attribute base value in PreAttributeChange */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	/** Clamp attributes Resource [0, 100] */
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	/** Broadcast events */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UFUNCTION()
	void OnRep_Resource(const FGameplayAttributeData& OldValue) const;

public:
	UECRPveCombatSet();

	ATTRIBUTE_ACCESSORS(UECRPveCombatSet, Resource);
};
