// Copyleft: All rights reversed

#include "Gameplay/GAS/Executions/ECRDamageExecution.h"
#include "GameplayEffectTypes.h"
#include "Gameplay/ECRGameplayBlueprintLibrary.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "System/ECRLogChannels.h"

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;
	FGameplayEffectAttributeCaptureDefinition ToughnessDef;
	FGameplayEffectAttributeCaptureDefinition IncomingDamageMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition ArmorDef;

	FDamageStatics()
	{
		// Common source attributes
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetBaseDamageAttribute(),
		                                                          EGameplayEffectAttributeCaptureSource::Source,
		                                                          true);
		ToughnessDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetToughnessAttribute(),
		                                                         EGameplayEffectAttributeCaptureSource::Target,
		                                                         true);
		IncomingDamageMultiplierDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetIncomingDamageMultiplierAttribute(),
		                                                         EGameplayEffectAttributeCaptureSource::Target,
		                                                         true);
		ArmorDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetArmorAttribute(),
		                                                     EGameplayEffectAttributeCaptureSource::Target,
		                                                     true);
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
}

void UECRDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FECRGameplayEffectContext* TypedContext = FECRGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	// ExecutionParams.GetOwningSpec().GetModifierMagnitude(0, false);

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
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
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
	}

	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);
	ToughnessAttenuation = FMath::Max(ToughnessAttenuation, 0.0f);
	TargetIncomingDamageMultiplier = FMath::Max(TargetIncomingDamageMultiplier, 0.0f);

	const float AttenuatedDamage = FMath::Max(
		0, BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * ToughnessAttenuation *
		TargetIncomingDamageMultiplier);
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UECRHealthSet::GetDamageAttribute(), EGameplayModOp::Additive,
		                               AttenuatedDamage));

#endif // #if WITH_SERVER_CODE
}
