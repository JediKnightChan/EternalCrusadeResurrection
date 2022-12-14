#include "Gameplay/GAS/Components/ECRCharacterHealthComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/ECRGameplayTags.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "System/Messages/ECRVerbMessage.h"
#include "System/Messages/ECRVerbMessageHelpers.h"


UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Wound_Message, "ECR.Wound.Message");


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

	CharacterHealthSet->OnReadyToBecomeWounded.AddUObject(this, &ThisClass::HandleReadyToBecomeWounded);
}

void UECRCharacterHealthComponent::UninitializeFromAbilitySystem()
{
	if (CharacterHealthSet)
	{
		CharacterHealthSet->OnReadyToBecomeWounded.RemoveAll(this);
	}
	CharacterHealthSet = nullptr;

	Super::UninitializeFromAbilitySystem();
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

void UECRCharacterHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		const FECRGameplayTags& GameplayTags = FECRGameplayTags::Get();

		AbilitySystemComponent->SetLooseGameplayTagCount(GameplayTags.Status_Wounded, 0);
	}
	Super::ClearGameplayTags();
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

void UECRCharacterHealthComponent::HandleReadyToBecomeWounded(AActor* DamageInstigator, AActor* DamageCauser,
                                                              const FGameplayEffectSpec& DamageEffectSpec,
                                                              float DamageMagnitude)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent)
	{
		// Send the "GameplayEvent.Wounded" gameplay event through the owner's ability system.  This can be used to trigger a death gameplay ability.
		{
			FGameplayEventData Payload;
			Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_Wounded;
			Payload.Instigator = DamageInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = DamageEffectSpec.Def;
			Payload.ContextHandle = DamageEffectSpec.GetEffectContext();
			Payload.InstigatorTags = *DamageEffectSpec.CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *DamageEffectSpec.CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = DamageMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}

		// Send a standardized verb message that other systems can observe
		{
			FECRVerbMessage Message;
			Message.Verb = TAG_ECR_Wound_Message;
			Message.Instigator = DamageInstigator;
			Message.InstigatorTags = *DamageEffectSpec.CapturedSourceTags.GetAggregatedTags();
			Message.Target = UECRVerbMessageHelpers::GetPlayerStateFromObject(AbilitySystemComponent->GetAvatarActor());
			Message.TargetTags = *DamageEffectSpec.CapturedTargetTags.GetAggregatedTags();
			//@TODO: Fill out context tags, and any non-ability-system source/instigator tags
			//@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...

			UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
			MessageSystem.BroadcastMessage(Message.Verb, Message);
		}

		//@TODO: assist messages (could compute from damage dealt elsewhere)?
	}

#endif // #if WITH_SERVER_CODE
}
