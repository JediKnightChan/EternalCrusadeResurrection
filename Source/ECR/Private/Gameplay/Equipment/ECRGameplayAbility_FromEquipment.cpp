// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Equipment/ECRGameplayAbility_FromEquipment.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"

UECRGameplayAbility_FromEquipment::UECRGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UECREquipmentInstance* UECRGameplayAbility_FromEquipment::GetAssociatedEquipment(UObject* SourceObject) const
{
	if (SourceObject)
	{
		return Cast<UECREquipmentInstance>(SourceObject);
	}
	if (FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UECREquipmentInstance>(Spec->SourceObject);
	}
	return nullptr;
}

UECRInventoryItemInstance* UECRGameplayAbility_FromEquipment::GetAssociatedItem(UObject* SourceObject) const
{
	if (UECREquipmentInstance* Equipment = GetAssociatedEquipment(SourceObject))
	{
		return Cast<UECRInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}


#if WITH_EDITOR
EDataValidationResult UECRGameplayAbility_FromEquipment::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);

	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		ValidationErrors.Add(NSLOCTEXT("ECR", "EquipmentAbilityMustBeInstanced",
		                               "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	if (AbilityTags.HasTag(FECRGameplayTags::Get().Ability_Behavior_SurvivesDeath))
	{
		ValidationErrors.Add(NSLOCTEXT("ECR", "EquipmentAbilityCannotSurviveDeath",
		                               "Equipment ability cannot have tag Ability_Behavior_SurvivesDeath"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif
