// Copyleft: All rights reversed

#include "Gameplay/GAS/Executions/ECRDamageExecution.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Gameplay/ECRGameplayBlueprintLibrary.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "System/ECRLogChannels.h"
#include "System/Messages/ECRVerbMessage.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_ECR_Reflect_Message, "ECR.Reflect.Message");

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;
	FGameplayEffectAttributeCaptureDefinition ToughnessDef;
	FGameplayEffectAttributeCaptureDefinition IncomingDamageMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition ArmorDef;
	FGameplayEffectAttributeCaptureDefinition OutgoingMeleeDamageMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition IncomingMeleeDamageMitigationDef;
	FGameplayEffectAttributeCaptureDefinition IncomingNonMeleeDamageMitigationDef;

	FDamageStatics()
	{
		// Common source attributes
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetBaseDamageAttribute(),
		                                                          EGameplayEffectAttributeCaptureSource::Source,
		                                                          true);
		ToughnessDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetToughnessAttribute(),
		                                                         EGameplayEffectAttributeCaptureSource::Target,
		                                                         false);
		IncomingDamageMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingDamageMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
		ArmorDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetArmorAttribute(),
		                                                     EGameplayEffectAttributeCaptureSource::Target,
		                                                     false);

		OutgoingMeleeDamageMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetOutgoingMeleeDamageMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Source,
			true);

		IncomingMeleeDamageMitigationDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingMeleeDamageMitigationAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
		IncomingNonMeleeDamageMitigationDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingNonMeleeDamageMitigationAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
	}
};

static FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}


UECRDamageExecution::UECRDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().ToughnessDef);
	RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageMultiplierDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().OutgoingMeleeDamageMultiplierDef);
	RelevantAttributesToCapture.Add(DamageStatics().IncomingMeleeDamageMitigationDef);
	RelevantAttributesToCapture.Add(DamageStatics().IncomingNonMeleeDamageMitigationDef);
}

void UECRDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FECRGameplayEffectContext* TypedContext = FECRGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);

	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);
	
	const UAbilitySystemComponent* TargetAsc = ExecutionParams.GetTargetAbilitySystemComponent();
	const UAbilitySystemComponent* SourceAsc = ExecutionParams.GetSourceAbilitySystemComponent();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters,
	                                                           BaseDamage);
	float TargetToughness = 100.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ToughnessDef, EvaluateParameters,
	                                                           TargetToughness);

	float TargetIncomingDamageMultiplier = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IncomingDamageMultiplierDef,
	                                                           EvaluateParameters,
	                                                           TargetIncomingDamageMultiplier);

	float TargetArmor = 100.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluateParameters,
	                                                           TargetArmor);

	float OutgoingMeleeDamageMultiplier = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().OutgoingMeleeDamageMultiplierDef,
	                                                           EvaluateParameters,
	                                                           OutgoingMeleeDamageMultiplier);

	float IncomingMeleeDamageMitigation = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IncomingMeleeDamageMitigationDef,
	                                                           EvaluateParameters,
	                                                           IncomingMeleeDamageMitigation);

	float IncomingNonMeleeDamageMitigation = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IncomingNonMeleeDamageMitigationDef,
	                                                           EvaluateParameters,
	                                                           IncomingNonMeleeDamageMitigation);

	const AActor* EffectCauser = TypedContext->GetEffectCauser();
	const FHitResult* HitActorResult = TypedContext->GetHitResult();

	AActor* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;

	// Calculation of hit actor, surface, zone, and distance all rely on whether the calculation has a hit result or not.
	// Effects just being added directly w/o having been targeted will always come in without a hit result, which must default
	// to some fallback information.
	if (HitActorResult)
	{
		const FHitResult& CurHitResult = *HitActorResult;
		HitActor = CurHitResult.HitObjectHandle.FetchActor();
		if (HitActor)
		{
			ImpactLocation = CurHitResult.ImpactPoint;
		}
	}

	// Handle case of no hit result or hit result not actually returning an actor
	if (!HitActor)
	{
		HitActor = TargetAsc ? TargetAsc->GetAvatarActor_Direct() : nullptr;
		if (HitActor)
		{
			ImpactLocation = HitActor->GetActorLocation();
		}
	}

	// Determine distance
	float Distance = WORLD_MAX;

	if (TypedContext->HasOrigin())
	{
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}
	else
	{
		UE_LOG(LogECRAbilitySystem, Warning,
		       TEXT(
			       "Damage Calculation cannot deduce a source location for damage coming from %s; "
			       "Falling back to WORLD_MAX dist!"
		       ), *GetPathNameSafe(Spec.Def))
	}

	// Apply ability source modifiers
	float PhysicalMaterialAttenuation = 1.0f;
	float DistanceAttenuation = 1.0f;
	float ToughnessAttenuation = 1.0f;
	bool IsDamageMelee = false;

	if (const IECRAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
		{
			PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(
				PhysMat, SourceTags, TargetTags);
		}

		DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);

		ToughnessAttenuation = UECRGameplayBlueprintLibrary::CalculateDamageAttenuationForArmorPenetration(
			AbilitySource->GetArmorPenetration(), TargetToughness, TargetArmor);
		IsDamageMelee = AbilitySource->GetIsDamageMelee();
	}

	if (IsDamageMelee)
	{
		TargetIncomingDamageMultiplier = TargetIncomingDamageMultiplier * OutgoingMeleeDamageMultiplier;
		TargetIncomingDamageMultiplier = TargetIncomingDamageMultiplier * IncomingMeleeDamageMitigation;
	}
	else
	{
		TargetIncomingDamageMultiplier = TargetIncomingDamageMultiplier * IncomingNonMeleeDamageMitigation;
	}

	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);
	ToughnessAttenuation = FMath::Max(ToughnessAttenuation, 0.0f);
	TargetIncomingDamageMultiplier = FMath::Max(TargetIncomingDamageMultiplier, 0.0f);

	float AttenuatedDamage = FMath::Max(
		0, BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * ToughnessAttenuation *
		TargetIncomingDamageMultiplier);

	// Special rules for damage
	if (SourceTags)
	{
		// Reduced damage only for 1 attack
		if (SourceTags->HasTag(FECRGameplayTags::Get().Gameplay_Special_ReducedDamage))
		{
			AttenuatedDamage = AttenuatedDamage * 0.75;
			if (UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent())
			{
				FGameplayTagContainer Container;
				Container.AddTag(FECRGameplayTags::Get().Gameplay_Special_ReducedDamage);
				SourceASC->RemoveActiveEffectsWithGrantedTags(Container);
			}
		}
	}
	if (TargetTags)
	{
		// Reflect
		if (TargetTags->HasTag(FECRGameplayTags::Get().Gameplay_Special_Reflect) && !AssetTags.HasTag(
			FECRGameplayTags::Get().GameplayEffect_NoReflect) && SourceAsc != TargetAsc)
		{
			double ReflectValue = 1.0f;
			if (TargetTags->HasTag(FECRGameplayTags::Get().Gameplay_Special_Reflect_50))
			{
				ReflectValue = 0.5f;
			}
			
			SendReflectMessage(Spec, TargetAsc ? TargetAsc->GetAvatarActor_Direct() : nullptr,
			                   AttenuatedDamage * ReflectValue);
		}
	}

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UECRHealthSet::GetDamageAttribute(), EGameplayModOp::Additive,
		                               AttenuatedDamage));

#endif // #if WITH_SERVER_CODE
}

void UECRDamageExecution::SendReflectMessage(const FGameplayEffectSpec& Spec, AActor* Target,
                                             double Magnitude) const
{
	if (AActor* Instigator = Spec.GetEffectContext().GetEffectCauser())
	{
		FECRVerbMessage Message;
		Message.Verb = TAG_ECR_Reflect_Message;
		Message.Instigator = Instigator;
		Message.InstigatorTags = *Spec.CapturedSourceTags.GetAggregatedTags();
		Message.Object1 = Spec.GetEffectContext().GetSourceObject();
		Message.Target = Target;
		Message.TargetTags = *Spec.CapturedTargetTags.GetAggregatedTags();
		Message.Magnitude = Magnitude;

		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(Instigator->GetWorld());
		MessageSystem.BroadcastMessage(Message.Verb, Message);
	}
}
