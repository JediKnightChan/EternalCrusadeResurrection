// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRAttributeSet.h"
#include "ECRCombatSet.generated.h"


/**
 *  Defines attributes that are necessary for applying damage or healing.
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

protected:
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue) const;
public:
	UECRCombatSet();

	ATTRIBUTE_ACCESSORS(UECRCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UECRCombatSet, BaseHeal);
};
