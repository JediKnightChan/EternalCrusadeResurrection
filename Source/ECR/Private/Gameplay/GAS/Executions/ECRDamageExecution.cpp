// Copyleft: All rights reversed

#include "Gameplay/GAS/Executions/ECRDamageExecution.h"
#include "GameplayEffectTypes.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/ECRAbilitySourceInterface.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Gameplay/GAS/Attributes/ECRSimpleHealthSet.h"

struct FDamageStatics
{
	// Common source attributes
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

	// Target character attributes
	FGameplayEffectAttributeCaptureDefinition CharacterHealthDef;
	FGameplayEffectAttributeCaptureDefinition CharacterShieldDef;

	// Target simple actor attributes
	FGameplayEffectAttributeCaptureDefinition SimpleActorHealthDef;

	FDamageStatics()
	{
		// Common source attributes
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetBaseDamageAttribute(),
		                                                          EGameplayEffectAttributeCaptureSource::Source,
		                                                          true);
		// Target character attributes
		CharacterHealthDef = FGameplayEffectAttributeCaptureDefinition(UECRCharacterHealthSet::GetHealthAttribute(),
		                                                               EGameplayEffectAttributeCaptureSource::Target,
		                                                               false);
		CharacterShieldDef = FGameplayEffectAttributeCaptureDefinition(UECRCharacterHealthSet::GetShieldAttribute(),
		                                                               EGameplayEffectAttributeCaptureSource::Target,
		                                                               false);

		// Target simple actor attributes
		SimpleActorHealthDef = FGameplayEffectAttributeCaptureDefinition(UECRSimpleHealthSet::GetHealthAttribute(),
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

	RelevantAttributesToCapture.Add(DamageStatics().CharacterHealthDef);
	RelevantAttributesToCapture.Add(DamageStatics().CharacterShieldDef);

	RelevantAttributesToCapture.Add(DamageStatics().SimpleActorHealthDef);

#if WITH_EDITORONLY_DATA
	InvalidScopedModifierAttributes.Add(DamageStatics().CharacterHealthDef);
	InvalidScopedModifierAttributes.Add(DamageStatics().CharacterShieldDef);
	InvalidScopedModifierAttributes.Add(DamageStatics().SimpleActorHealthDef);
#endif // #if WITH_EDITORONLY_DATA
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
	UE_LOG(LogTemp, Warning, TEXT("Damage base is %f"), BaseDamage);
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
		UE_LOG(LogTemp, Warning,
		       TEXT(
			       "Damage Calculation cannot deduce a source location for damage coming from %s; "
			       "Falling back to WORLD_MAX dist!"
		       ), *GetPathNameSafe(Spec.Def))
	}

	// Apply ability source modifiers
	float PhysicalMaterialAttenuation = 1.0f;
	float DistanceAttenuation = 1.0f;
	if (const IECRAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
		{
			PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(
				PhysMat, SourceTags, TargetTags);
		}

		DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
	}
	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);

	const float AttenuatedDamage = BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation;

	// Or maybe ExecutionParams.GetTargetAbilitySystemComponent()->GetAttributeSet(UECRCharacterHealthSet::StaticClass());
	if (Cast<AECRCharacter>(HitActor))
	{
		// Handling applying damage to character
		ApplyDamageToCharacter(ExecutionParams, OutExecutionOutput, EvaluateParameters, AttenuatedDamage);
	}
	else
	{
		ApplyDamageToSimpleActor(ExecutionParams, OutExecutionOutput, EvaluateParameters, AttenuatedDamage);
	}

#endif // #if WITH_SERVER_CODE
}


void UECRDamageExecution::ApplyDamageToCharacter(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                 FGameplayEffectCustomExecutionOutput& OutExecutionOutput,
                                                 const FAggregatorEvaluateParameters EvaluateParameters,
                                                 const float AttenuatedDamage) const
{
	float CurrentHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CharacterHealthDef, EvaluateParameters,
	                                                           CurrentHealth);

	float CurrentShield = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CharacterShieldDef, EvaluateParameters,
	                                                           CurrentShield);

	if (CurrentShield > 0)
	{
		// Apply damage to shield

		// This clamp prevents us from doing more damage than there is shield available.
		const float DamageDone = FMath::Clamp(AttenuatedDamage, 0.0f, CurrentShield);
		if (DamageDone > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(UECRCharacterHealthSet::GetShieldAttribute(), EGameplayModOp::Additive,
				                               -DamageDone));
		}
	}
	else
	{
		// Apply damage to health

		// This clamp prevents us from doing more damage than there is health available.
		const float DamageDone = FMath::Clamp(AttenuatedDamage, 0.0f, CurrentHealth);
		if (DamageDone > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(UECRCharacterHealthSet::GetHealthAttribute(), EGameplayModOp::Additive,
				                               -DamageDone));
		}
	}
}

void UECRDamageExecution::ApplyDamageToSimpleActor(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                   FGameplayEffectCustomExecutionOutput& OutExecutionOutput,
                                                   const FAggregatorEvaluateParameters EvaluateParameters,
                                                   const float AttenuatedDamage) const
{
	float CurrentHealth = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().SimpleActorHealthDef, EvaluateParameters,
	                                                           CurrentHealth);

	// This clamp prevents us from doing more damage than there is health available.
	const float DamageDone = FMath::Clamp(AttenuatedDamage, 0.0f, CurrentHealth);
	if (DamageDone > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(UECRSimpleHealthSet::GetHealthAttribute(), EGameplayModOp::Additive,
			                               -DamageDone));
	}
}
