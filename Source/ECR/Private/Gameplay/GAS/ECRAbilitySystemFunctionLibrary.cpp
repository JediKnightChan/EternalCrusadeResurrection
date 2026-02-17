#include "Gameplay/GAS/ECRAbilitySystemFunctionLibrary.h"

#include "AbilitySystemComponent.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectUIData.h"
#include "Gameplay/GAS/Abilities/ECRGameplayAbility.h"


struct FECRGameplayEffectContext;

FGameplayCueParameters UECRAbilitySystemFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSource(
	const FHitResult& HitResult)
{
	FGameplayCueParameters CueParameters =
		UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(HitResult);
	CueParameters.SourceObject = HitResult.GetActor();
	return CueParameters;
}

FGameplayCueParameters UECRAbilitySystemFunctionLibrary::MakeGameplayCueParametersFromHitResultIncludingSourceAndCauser(
	const FHitResult& HitResult, AActor* Causer)
{
	FGameplayCueParameters CueParameters = MakeGameplayCueParametersFromHitResultIncludingSource(HitResult);
	CueParameters.EffectCauser = MakeWeakObjectPtr(Cast<AActor>(Causer));
	return CueParameters;
}

void UECRAbilitySystemFunctionLibrary::SetEffectContextSourceObject(FGameplayEffectContextHandle Handle,
                                                                    UObject* Object)
{
	Handle.AddSourceObject(Object);
}

FECRGameplayModifierInfoWrapper UECRAbilitySystemFunctionLibrary::ExtractGameplayModifierInfo(
	FGameplayModifierInfo Info)
{
	FECRGameplayModifierInfoWrapper InfoWrapper;

	InfoWrapper.Attribute = Info.Attribute;
	InfoWrapper.ModifierOp = Info.ModifierOp;
	Info.ModifierMagnitude.GetStaticMagnitudeIfPossible(1, InfoWrapper.Magnitude);
	return InfoWrapper;
}

FECRGameplayModifierInfoWrapper UECRAbilitySystemFunctionLibrary::ExtractGameplayModifierInfoFromExecution(
	FGameplayEffectExecutionDefinition Info, int32 Index)
{
	if (Index >= 0 && Index < Info.CalculationModifiers.Num())
	{
		FECRGameplayModifierInfoWrapper InfoWrapper;
		FGameplayEffectExecutionScopedModifierInfo OutInfo = Info.CalculationModifiers[Index];
		InfoWrapper.ModifierOp = OutInfo.ModifierOp;
		OutInfo.ModifierMagnitude.GetStaticMagnitudeIfPossible(1, InfoWrapper.Magnitude);
		return InfoWrapper;
	}
	return FECRGameplayModifierInfoWrapper{};
}

float UECRAbilitySystemFunctionLibrary::ExtractDurationFromGameplayEffect(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!EffectClass)
	{
		return 0;
	}

	UGameplayEffect* EffectCDO = EffectClass->GetDefaultObject<UGameplayEffect>();
	if (!EffectCDO)
	{
		return 0;
	}

	if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		return 0;
	}
	if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Infinite)
	{
		return -1;
	}
	float OutValue = 0;
	EffectCDO->DurationMagnitude.GetStaticMagnitudeIfPossible(1, OutValue);
	return OutValue;
}

const UGameplayEffectUIData* UECRAbilitySystemFunctionLibrary::GetGameplayAbilityUIData(
	TSubclassOf<UECRGameplayAbility> AbilityClass, TSubclassOf<UGameplayEffectUIData> DataType)
{
	if (UClass* ActualPtr = AbilityClass.Get())
	{
		const UGameplayEffectUIData* UIData = GetDefault<UECRGameplayAbility>(ActualPtr)->UIData;
		if ((UIData != nullptr) && (DataType != nullptr) && UIData->IsA(DataType))
		{
			return UIData;
		}
	}
	return nullptr;
}

int64 UECRAbilitySystemFunctionLibrary::ExtractIdFromEffectHandle(FActiveGameplayEffectHandle Handle)
{
	return static_cast<int64>(GetTypeHash(Handle));
}

UObject* UECRAbilitySystemFunctionLibrary::ExtractSourceFromEffectHandle(FActiveGameplayEffectHandle Handle)
{
	UAbilitySystemComponent* OwningAbilitySystemComponent = Handle.GetOwningAbilitySystemComponent();
	if (OwningAbilitySystemComponent)
	{
		FGameplayEffectContextHandle EffectContext = OwningAbilitySystemComponent->GetEffectContextFromActiveGEHandle(Handle);
		return EffectContext.GetSourceObject();
	}
	return nullptr;
}

UObject* UECRAbilitySystemFunctionLibrary::ExtractInstigatorFromEffectHandle(FActiveGameplayEffectHandle Handle)
{
	UAbilitySystemComponent* OwningAbilitySystemComponent = Handle.GetOwningAbilitySystemComponent();
	if (OwningAbilitySystemComponent)
	{
		FGameplayEffectContextHandle EffectContext = OwningAbilitySystemComponent->GetEffectContextFromActiveGEHandle(Handle);
		return EffectContext.GetInstigator();
	}
	return nullptr;
}

UObject* UECRAbilitySystemFunctionLibrary::ExtractEffectCauserFromEffectHandle(FActiveGameplayEffectHandle Handle)
{
	UAbilitySystemComponent* OwningAbilitySystemComponent = Handle.GetOwningAbilitySystemComponent();
	if (OwningAbilitySystemComponent)
	{
		FGameplayEffectContextHandle EffectContext = OwningAbilitySystemComponent->GetEffectContextFromActiveGEHandle(Handle);
		return EffectContext.GetEffectCauser();
	}
	return nullptr;
}

bool UECRAbilitySystemFunctionLibrary::GetIsAbilityActive(UAbilitySystemComponent* AbilitySystem, FGameplayAbilitySpecHandle Handle)
{
	if (AbilitySystem)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromHandle(Handle);
		if (AbilitySpec)
		{
			return AbilitySpec->IsActive();
		}
	}
	return false;
}

bool UECRAbilitySystemFunctionLibrary::GetIsAbilityActivatable(UAbilitySystemComponent* AbilitySystem,
	FGameplayAbilitySpecHandle Handle, bool bIgnoreTags)
{
	if (AbilitySystem)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromHandle(Handle);
		if (AbilitySpec)
		{
			TObjectPtr<UGameplayAbility> GameplayAbility = AbilitySpec->Ability;
			if (GameplayAbility)
			{
				if (const FGameplayAbilityActorInfo* ActorInfo = AbilitySystem->AbilityActorInfo.Get())
				{
					if (bIgnoreTags)
					{
						if (UECRGameplayAbility* EcrAbility = Cast<UECRGameplayAbility>(GameplayAbility))
						{
							return EcrAbility->CanActivateAbilityNotMindingTags(Handle, ActorInfo);
						}
					}
					FGameplayTagContainer OutTags;
					AbilitySystem->GetOwnedGameplayTags(OutTags);
					return GameplayAbility->CanActivateAbility(Handle, ActorInfo, &OutTags);
				}
			}
		}
	}
	return false;
}

float UECRAbilitySystemFunctionLibrary::GetAbilityTotalCooldown(UAbilitySystemComponent* AbilitySystem,
                                                                FGameplayAbilitySpecHandle Handle)
{
	if (AbilitySystem)
	{
		FGameplayAbilitySpec* AbilitySpec = AbilitySystem->FindAbilitySpecFromHandle(Handle);
		if (AbilitySpec)
		{
			TObjectPtr<UGameplayAbility> GameplayAbility = AbilitySpec->Ability;
			if (GameplayAbility)
			{
				const FGameplayTagContainer* CooldownTags = GameplayAbility->GetCooldownTags();
				if (CooldownTags && CooldownTags->Num() > 0)
				{
					FGameplayEffectQuery const Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags);
					TArray< TPair<float,float> > DurationAndTimeRemaining = AbilitySystem->GetActiveEffectsTimeRemainingAndDuration(Query);
					if (DurationAndTimeRemaining.Num() > 0)
					{
						int32 BestIdx = 0;
						float LongestTime = DurationAndTimeRemaining[0].Key;
						for (int32 Idx = 1; Idx < DurationAndTimeRemaining.Num(); ++Idx)
						{
							if (DurationAndTimeRemaining[Idx].Key > LongestTime)
							{
								LongestTime = DurationAndTimeRemaining[Idx].Key;
								BestIdx = Idx;
							}
						}
						return DurationAndTimeRemaining[BestIdx].Value;
					}
				}
			}
		}
	}
	return 0.0f;
}
