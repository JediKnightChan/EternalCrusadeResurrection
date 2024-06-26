// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRAttributeSet.h"
#include "ECRCombatSet.generated.h"


/**
 *  Defines attributes that are always necessary for applying / receiving damage or healing.
 */
UCLASS(BlueprintType)
class UECRCombatSet : public UECRAttributeSet
{
	GENERATED_BODY()

	// The base amount of damage to apply in the damage execution. Usually overriden in GE.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_BaseDamage, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution. Usually overriden in GE.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_BaseHeal, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData BaseHeal;

	// Higher (Toughness - WeaponArmorPenetration) -> more damage reduction
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Toughness, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Toughness;
	
	// If Armor > WeaponArmorPenetration, then no damage at all
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Armor, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData Armor;

	// Spread multiplier for shooting
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SpreadMultiplier, Category="Attributes",
		Meta=(AllowPrivateAccess="true"))
	FGameplayAttributeData SpreadMultiplier;

protected:
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_Toughness(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_SpreadMultiplier(const FGameplayAttributeData& OldValue) const;
public:
	UECRCombatSet();

	ATTRIBUTE_ACCESSORS(UECRCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UECRCombatSet, BaseHeal);
	ATTRIBUTE_ACCESSORS(UECRCombatSet, Toughness);
	ATTRIBUTE_ACCESSORS(UECRCombatSet, Armor);
	ATTRIBUTE_ACCESSORS(UECRCombatSet, SpreadMultiplier);
};
