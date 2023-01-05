#include "Gameplay/GAS/Components/ECRCharacterHealthComponent.h"

#include "System/ECRLogChannels.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"

void UECRCharacterHealthComponent::InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC)
{
	Super::InitializeWithAbilitySystem(InASC);

	CharacterHealthSet = Cast<UECRCharacterHealthSet>(HealthSet);
	if (!CharacterHealthSet)
	{
		UE_LOG(LogECR, Error,
		       TEXT(
			       "ECRHealthComponent: Cannot initialize character health component for owner [%s] with NULL health set on the ability system."
		       ), *GetNameSafe(GetOwner()));
		return;
	}

	// Shield
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRCharacterHealthSet::GetShieldAttribute()).
	                        AddUObject(this, &ThisClass::HandleShieldChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRCharacterHealthSet::GetMaxShieldAttribute()).
	                        AddUObject(this, &ThisClass::HandleMaxShieldChanged);
	OnShieldChanged.Broadcast(this, CharacterHealthSet->GetShield(), CharacterHealthSet->GetShield(), nullptr);
	OnMaxShieldChanged.Broadcast(this, CharacterHealthSet->GetMaxShield(), CharacterHealthSet->GetMaxShield(), nullptr);

	// Stamina
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRCharacterHealthSet::GetStaminaAttribute()).
	                        AddUObject(this, &ThisClass::HandleStaminaChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRCharacterHealthSet::GetMaxStaminaAttribute()).
	                        AddUObject(this, &ThisClass::HandleMaxStaminaChanged);
	OnStaminaChanged.Broadcast(this, CharacterHealthSet->GetStamina(), CharacterHealthSet->GetStamina(), nullptr);
	OnMaxStaminaChanged.Broadcast(this, CharacterHealthSet->GetMaxStamina(), CharacterHealthSet->GetMaxStamina(),
	                              nullptr);
}

void UECRCharacterHealthComponent::UninitializeFromAbilitySystem()
{
	Super::UninitializeFromAbilitySystem();
	CharacterHealthSet = nullptr;
}

float UECRCharacterHealthComponent::GetShield() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetShield() : 0.0f);
}

float UECRCharacterHealthComponent::GetMaxShield() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetMaxShield() : 0.0f);
}

float UECRCharacterHealthComponent::GetShieldNormalized() const
{
	if (CharacterHealthSet)
	{
		const float Shield = CharacterHealthSet->GetShield();
		const float MaxShield = CharacterHealthSet->GetMaxShield();

		return ((MaxShield > 0.0f) ? (Shield / MaxShield) : 0.0f);
	}

	return 0.0f;
}

float UECRCharacterHealthComponent::GetStamina() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetMaxStamina() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetMaxStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetStaminaNormalized() const
{
	if (CharacterHealthSet)
	{
		const float Stamina = CharacterHealthSet->GetStamina();
		const float MaxStamina = CharacterHealthSet->GetMaxStamina();

		return ((MaxStamina > 0.0f) ? (Stamina / MaxStamina) : 0.0f);
	}

	return 0.0f;
}

void UECRCharacterHealthComponent::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	OnShieldChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                          GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleMaxShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxShieldChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                             GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	OnStaminaChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                           GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxStaminaChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                              GetInstigatorFromAttrChangeData(ChangeData));
}
