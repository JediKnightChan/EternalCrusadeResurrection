// Copyleft: All rights reversed


#include "Gameplay/GAS/Attributes/ECRHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "System/Messages/ECRVerbMessage.h"
#include "GameFramework/GameplayMessageSubsystem.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_Damage, "Gameplay.Damage");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageImmunity, "Gameplay.DamageImmunity");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_DamageSelfDestruct, "Gameplay.Damage.Reason.SelfDestruct");
UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_FellOutOfWorld, "Gameplay.Damage.Reason.FellOutOfWorld");
UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Damage_Message, "ECR.Damage.Message");


UECRHealthSet::UECRHealthSet()
	: Health(100.0f),
	  MaxHealth(100.0f)
{
	bReadyToDie = false;
}


bool UECRHealthSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		if (Data.EvaluatedData.Magnitude > 0.0f)
		{
			const bool bIsDamageFromSelfDestruct = Data.EffectSpec.GetDynamicAssetTags().HasTagExact(
				TAG_Gameplay_DamageSelfDestruct);

			if (Data.Target.HasMatchingGameplayTag(TAG_Gameplay_DamageImmunity) && !bIsDamageFromSelfDestruct)
			{
				// Do not take away any health.
				Data.EvaluatedData.Magnitude = 0.0f;
				return false;
			}
		}
	}

	return true;
}


void UECRHealthSet::SendDamageMessage(const FGameplayEffectModCallbackData& DamageData) const
{
	FECRVerbMessage Message;
	Message.Verb = TAG_ECR_Damage_Message;
	Message.Instigator = DamageData.EffectSpec.GetEffectContext().GetEffectCauser();
	Message.InstigatorTags = *DamageData.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	Message.Target = GetOwningActor();
	Message.TargetTags = *DamageData.EffectSpec.CapturedTargetTags.GetAggregatedTags();
	//@TODO: Fill out context tags, and any non-ability-system source/instigator tags
	//@TODO: Determine if it's an opposing team kill, self-own, team kill, etc...
	Message.Magnitude = DamageData.EvaluatedData.Magnitude;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(Message.Verb, Message);
}


bool UECRHealthSet::GetIsReadyToDie() const
{
	return GetHealth() <= 0.0f;
}

void UECRHealthSet::CheckIfReadyToDie(const FGameplayEffectModCallbackData& Data)
{
	if (GetIsReadyToDie() && !bReadyToDie)
	{
		if (OnReadyToDie.IsBound())
		{
			const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
			AActor* Instigator = EffectContext.GetOriginalInstigator();
			AActor* Causer = EffectContext.GetEffectCauser();

			OnReadyToDie.Broadcast(Instigator, Causer, Data.EffectSpec, Data.EvaluatedData.Magnitude);
		}
	}

	// Check health again in case an event above changed it.
	bReadyToDie = GetIsReadyToDie();
}

void UECRHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Send a standardized verb message that other systems can observe
		if (Data.EvaluatedData.Magnitude < 0.0f)
		{
			SendDamageMessage(Data);
		}

		// Convert into -Health and then clamp
		SetHealth(FMath::Clamp(GetHealth() - GetDamage(), 0, GetMaxHealth()));
		SetDamage(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth() + GetHealing(), 0, GetMaxHealth()));
		SetHealing(0.0f);
	}

	CheckIfReadyToDie(Data);
}


void UECRHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}


void UECRHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Make sure current health is not greater than the new max health.
	ClampCurrentAttributeOnMaxChange(Attribute, NewValue, GetMaxHealthAttribute(),
	                                 GetHealthAttribute(), GetHealth());

	if (bReadyToDie && (GetHealth() > 0.0f))
	{
		bReadyToDie = false;
	}
}


void UECRHealthSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetHealthAttribute())
	{
		// Do not allow health to go negative or above max health.
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// Do not allow max health to drop below 1.
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}


void UECRHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UECRHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UECRHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}


void UECRHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRHealthSet, Health, OldValue);
}


void UECRHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UECRHealthSet, MaxHealth, OldValue);
}
