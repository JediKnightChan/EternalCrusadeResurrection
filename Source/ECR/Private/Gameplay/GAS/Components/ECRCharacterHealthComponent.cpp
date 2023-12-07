#include "Gameplay/GAS/Components/ECRCharacterHealthComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/ECRGameplayTags.h"
#include "System/ECRLogChannels.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRMovementSet.h"
#include "System/Messages/ECRVerbMessage.h"
#include "System/Messages/ECRVerbMessageHelpers.h"


UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ECR_Wound_Message, "ECR.Wound.Message");


void UECRCharacterHealthComponent::InitializeWithAbilitySystem(UECRAbilitySystemComponent* InASC)
{
	Super::InitializeWithAbilitySystem(InASC);

	CharacterHealthSet = Cast<UECRCharacterHealthSet>(HealthSet);
	CharacterMovementSet = AbilitySystemComponent->GetSet<UECRMovementSet>();
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
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRCharacterHealthSet::GetMaxShieldAttribute())
	                      .
	                      AddUObject(this, &ThisClass::HandleMaxShieldChanged);
	OnShieldChanged.Broadcast(this, CharacterHealthSet->GetShield(), CharacterHealthSet->GetShield(), nullptr);
	OnMaxShieldChanged.Broadcast(this, CharacterHealthSet->GetMaxShield(), CharacterHealthSet->GetMaxShield(),
	                             nullptr);

	// Bleeding health
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		                        UECRCharacterHealthSet::GetBleedingHealthAttribute()).
	                        AddUObject(this, &ThisClass::HandleBleedingHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		                        UECRCharacterHealthSet::GetMaxBleedingHealthAttribute()).
	                        AddUObject(this, &ThisClass::HandleMaxBleedingHealthChanged);
	OnBleedingHealthChanged.Broadcast(this, CharacterHealthSet->GetBleedingHealth(),
	                                  CharacterHealthSet->GetBleedingHealth(), nullptr);
	OnMaxBleedingHealthChanged.Broadcast(this, CharacterHealthSet->GetMaxBleedingHealth(),
	                                     CharacterHealthSet->GetMaxBleedingHealth(),
	                                     nullptr);

	CharacterHealthSet->OnReadyToBecomeWounded.AddUObject(this, &ThisClass::HandleReadyToBecomeWounded);
	CharacterHealthSet->OnReadyToBecomeUnwounded.AddUObject(this, &ThisClass::HandleReadyToBecomeUnwounded);


	if (CharacterMovementSet)
	{
		// Root motion scale
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			UECRMovementSet::GetRootMotionScaleAttribute()).AddUObject(
			this, &ThisClass::HandleRootMotionScaleChanged);
		ChangeCharacterRootMotionScale(CharacterMovementSet->GetRootMotionScale());

		// Speed
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRMovementSet::GetWalkSpeedAttribute()).
		                        AddUObject(this, &ThisClass::HandleWalkSpeedChanged);
		ChangeCharacterSpeed(CharacterMovementSet->GetWalkSpeed());

		// Stamina
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRMovementSet::GetStaminaAttribute()).
		                        AddUObject(this, &ThisClass::HandleStaminaChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UECRMovementSet::GetMaxStaminaAttribute()).
		                        AddUObject(this, &ThisClass::HandleMaxStaminaChanged);
		OnStaminaChanged.Broadcast(this, CharacterMovementSet->GetStamina(), CharacterMovementSet->GetStamina(),
		                           nullptr);
		OnMaxStaminaChanged.Broadcast(this, CharacterMovementSet->GetMaxStamina(),
		                              CharacterMovementSet->GetMaxStamina(),
		                              nullptr);

		// Evasion stamina
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			                        UECRMovementSet::GetEvasionStaminaAttribute()).
		                        AddUObject(this, &ThisClass::HandleEvasionStaminaChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			                        UECRMovementSet::GetMaxEvasionStaminaAttribute()).
		                        AddUObject(this, &ThisClass::HandleMaxEvasionStaminaChanged);
		OnEvasionStaminaChanged.Broadcast(this, CharacterMovementSet->GetEvasionStamina(),
		                                  CharacterMovementSet->GetEvasionStamina(), nullptr);
		OnMaxEvasionStaminaChanged.Broadcast(this, CharacterMovementSet->GetMaxEvasionStamina(),
		                                     CharacterMovementSet->GetMaxEvasionStamina(), nullptr);
	}
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
	return GetNormalizedAttributeValue(GetShield(), GetMaxShield());
}

float UECRCharacterHealthComponent::GetBleedingHealth() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetBleedingHealth() : 0.0f);
}

float UECRCharacterHealthComponent::GetMaxBleedingHealth() const
{
	return (CharacterHealthSet ? CharacterHealthSet->GetMaxBleedingHealth() : 0.0f);
}

float UECRCharacterHealthComponent::GetBleedingHealthNormalized() const
{
	return GetNormalizedAttributeValue(GetBleedingHealth(), GetMaxBleedingHealth());
}

float UECRCharacterHealthComponent::GetStamina() const
{
	return (CharacterMovementSet ? CharacterMovementSet->GetStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetMaxStamina() const
{
	return (CharacterMovementSet ? CharacterMovementSet->GetMaxStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetStaminaNormalized() const
{
	return GetNormalizedAttributeValue(GetStamina(), GetMaxStamina());
}

float UECRCharacterHealthComponent::GetEvasionStamina() const
{
	return (CharacterMovementSet ? CharacterMovementSet->GetEvasionStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetMaxEvasionStamina() const
{
	return (CharacterMovementSet ? CharacterMovementSet->GetMaxEvasionStamina() : 0.0f);
}

float UECRCharacterHealthComponent::GetEvasionStaminaNormalized() const
{
	return GetNormalizedAttributeValue(GetEvasionStamina(), GetMaxEvasionStamina());
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


void UECRCharacterHealthComponent::ChangeCharacterRootMotionScale(float NewScale)
{
	if (AActor* Actor = GetOwner())
	{
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			Character->SetAnimRootMotionTranslationScale(NewScale);
		}
	}
}


void UECRCharacterHealthComponent::ChangeCharacterSpeed(const float NewSpeed)
{
	if (AActor* Actor = GetOwner())
	{
		if (const ACharacter* Character = Cast<ACharacter>(Actor))
		{
			UCharacterMovementComponent* CharMoveComp = Character->GetCharacterMovement();
			CharMoveComp->MaxWalkSpeed = NewSpeed;
		}
	}
}

void UECRCharacterHealthComponent::HandleRootMotionScaleChanged(const FOnAttributeChangeData& ChangeData)
{
	const float NewScale = ChangeData.NewValue;
	ChangeCharacterRootMotionScale(NewScale);
}

void UECRCharacterHealthComponent::HandleWalkSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	const float NewSpeed = ChangeData.NewValue;
	ChangeCharacterSpeed(NewSpeed);
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

void UECRCharacterHealthComponent::HandleBleedingHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnBleedingHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                                  GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleMaxBleedingHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxBleedingHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                                     GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleReadyToBecomeWounded(AActor* DamageInstigator, AActor* DamageCauser,
                                                              const FGameplayEffectSpec& DamageEffectSpec,
                                                              float DamageMagnitude)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent)
	{
		// Send the "GameplayEvent.Wounded" gameplay event through the owner's ability system.
		// This can be used to trigger Wounded gameplay ability.
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


void UECRCharacterHealthComponent::HandleReadyToBecomeUnwounded(AActor* EffectInstigator, AActor* EffectCauser,
                                                                const FGameplayEffectSpec& EffectSpec,
                                                                float EffectMagnitude)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent)
	{
		// Send the "GameplayEvent.WoundedCancel" gameplay event through the owner's ability system.
		{
			FGameplayEventData Payload;
			Payload.EventTag = FECRGameplayTags::Get().GameplayEvent_WoundedCancel;
			Payload.Instigator = EffectInstigator;
			Payload.Target = AbilitySystemComponent->GetAvatarActor();
			Payload.OptionalObject = EffectSpec.Def;
			Payload.ContextHandle = EffectSpec.GetEffectContext();
			Payload.InstigatorTags = *EffectSpec.CapturedSourceTags.GetAggregatedTags();
			Payload.TargetTags = *EffectSpec.CapturedTargetTags.GetAggregatedTags();
			Payload.EventMagnitude = EffectMagnitude;

			FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
			AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		}
	}
#endif
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

void UECRCharacterHealthComponent::HandleEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	OnEvasionStaminaChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                                  GetInstigatorFromAttrChangeData(ChangeData));
}

void UECRCharacterHealthComponent::HandleMaxEvasionStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	OnMaxEvasionStaminaChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue,
	                                     GetInstigatorFromAttrChangeData(ChangeData));
}

float UECRCharacterHealthComponent::GetDamageToKill()
{
	return GetMaxShield() + GetMaxHealth() + GetMaxBleedingHealth();
}
