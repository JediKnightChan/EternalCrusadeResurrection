// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GAS/Abilities/ECRAbilityCost_PlayerTagStack.h"
#include "Gameplay/Player/ECRPlayerState.h"
#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"
#include "GameFramework/PlayerController.h"

UECRAbilityCost_PlayerTagStack::UECRAbilityCost_PlayerTagStack()
{
	Quantity.SetValue(1.0f);
}

bool UECRAbilityCost_PlayerTagStack::CheckCost(const UECRGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (AController* PC = Ability->GetControllerFromActorInfo())
	{
		if (AECRPlayerState* PS = Cast<AECRPlayerState>(PC->PlayerState))
		{
			const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

			const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
			const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

			return PS->GetStatTagStackCount(Tag) >= NumStacks;
		}
	}
	return false;
}

void UECRAbilityCost_PlayerTagStack::ApplyCost(const UECRGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ActorInfo->IsNetAuthority())
	{
		if (AController* PC = Ability->GetControllerFromActorInfo())
		{
			if (AECRPlayerState* PS = Cast<AECRPlayerState>(PC->PlayerState))
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				PS->RemoveStatTagStack(Tag, NumStacks);
			}
		}
	}
}
