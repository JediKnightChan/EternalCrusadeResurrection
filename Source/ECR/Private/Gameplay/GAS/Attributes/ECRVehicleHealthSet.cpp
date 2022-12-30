// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRVehicleHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UECRVehicleHealthSet::UECRVehicleHealthSet()
	: Engine_Health(100.0f),
	  Transmission_Health(100.0f),
	  CannonBreech_Health(100.0f),
	  CannonBarrel_Health(100.0f),
	  Radiator_Health(100.0f),
	  FuelTank_Health(100.0f),
	  Track1_Health(100.0f),
	  Track2_Health(100.0f),
	  AmmoRack_Health(100.0f)
{
	HealthAttributes = {
		GetEngine_HealthAttribute(),
		GetTransmission_HealthAttribute(),
		GetCannonBreech_HealthAttribute(),
		GetCannonBarrel_HealthAttribute(),
		GetRadiator_HealthAttribute(),
		GetFuelTank_HealthAttribute(),
		GetTrack1_HealthAttribute(),
		GetTrack2_HealthAttribute(),
		GetAmmoRack_HealthAttribute()
	};
}


bool UECRVehicleHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (HealthAttributes.Contains(Data.EvaluatedData.Attribute))
	{
		if (Data.EvaluatedData.Magnitude < 0.0f)
		{
			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(
				TAG_Gameplay_DamageSelfDestruct);

			if (Data.Target.HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any shield.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
		}
	}

	return true;
}

void UECRVehicleHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRVehicleHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRVehicleHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (HealthAttributes.Contains(Attribute))
	{
		// Do not allow any health to go negative
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}


void UECRVehicleHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, Engine_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, Transmission_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, CannonBreech_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, CannonBarrel_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, Radiator_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, FuelTank_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, Track1_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, Track2_Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRVehicleHealthSet, AmmoRack_Health, COND_None, REPNOTIFY_Always);
}


void UECRVehicleHealthSet::OnRep_Engine_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, Engine_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_Transmission_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, Transmission_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_CannonBreech_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, CannonBreech_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_CannonBarrel_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, CannonBarrel_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_Radiator_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, Radiator_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_FuelTank_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, FuelTank_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_Track1_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, Track1_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_Track2_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, Track2_Health, OldValue)
}


void UECRVehicleHealthSet::OnRep_AmmoRack_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRVehicleHealthSet, AmmoRack_Health, OldValue)
}
