// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Abilities/ECRAbilityCost_ItemTagStack.h"

#include "AbilitySystemComponent.h"
#include "Gameplay/Equipment/ECREquipmentInstance.h"
#include "Gameplay/Inventory/ECRInventoryItemInstance.h"
#include "Gameplay/Equipment/ECRGameplayAbility_FromEquipment.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ABILITY_FAIL_COST, "Ability.ActivateFail.Cost");

UECRAbilityCost_ItemTagStack::UECRAbilityCost_ItemTagStack()
{
	Quantity.SetValue(1.0f);
	FailureTag = TAG_ABILITY_FAIL_COST;
}

bool UECRAbilityCost_ItemTagStack::CheckCost(const UECRGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (const UECRGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UECRGameplayAbility_FromEquipment>(Ability))
	{
		const FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent.Get()->FindAbilitySpecFromHandle(Handle);
		if (const UECRInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem(AbilitySpec->SourceObject.Get()))
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
			const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;
			// Inform other abilities why this cost cannot be applied
			if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
			{
				OptionalRelevantTags->AddTag(FailureTag);				
			}
			return bCanApplyCost;
		}
	}
	return false;
}

void UECRAbilityCost_ItemTagStack::ApplyCost(const UECRGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (const UECRGameplayAbility_FromEquipment* EquipmentAbility = Cast<const UECRGameplayAbility_FromEquipment>(Ability))
		{
			if (UECRInventoryItemInstance* ItemInstance = EquipmentAbility->GetAssociatedItem())
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
				ItemInstance->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}
