// Copyleft: All rights reversed

#include "Gameplay/GAS/Executions/ECRHealingExecution.h"
#include "GameplayEffectTypes.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/ECRGameplayEffectContext.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"

struct FHealingStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseHealingDef;
	FGameplayEffectAttributeCaptureDefinition OutgoingHealingMultiplierDef;
	FGameplayEffectAttributeCaptureDefinition IncomingHealingMultiplierDef;

	FHealingStatics()
	{
		// Common source attributes
		BaseHealingDef = FGameplayEffectAttributeCaptureDefinition(UECRCombatSet::GetBaseHealAttribute(),
		                                                           EGameplayEffectAttributeCaptureSource::Source,
		                                                           true);
		OutgoingHealingMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetOutgoingHealingMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Source,
			false);
		IncomingHealingMultiplierDef = FGameplayEffectAttributeCaptureDefinition(
			UECRCombatSet::GetIncomingHealingMultiplierAttribute(),
			EGameplayEffectAttributeCaptureSource::Target,
			false);
	}
};

static FHealingStatics& HealingStatics()
{
	static FHealingStatics Statics;
	return Statics;
}


UECRHealingExecution::UECRHealingExecution()
{
	RelevantAttributesToCapture.Add(HealingStatics().BaseHealingDef);
	RelevantAttributesToCapture.Add(HealingStatics().IncomingHealingMultiplierDef);
	RelevantAttributesToCapture.Add(HealingStatics().OutgoingHealingMultiplierDef);
}

void UECRHealingExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                                  FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FECRGameplayEffectContext* TypedContext = FECRGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	check(TypedContext);

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	// ExecutionParams.GetOwningSpec().GetModifierMagnitude(0, false);

	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseHealing = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealingStatics().BaseHealingDef, EvaluateParameters,
	                                                           BaseHealing);

	float SourceOutgoingHealingMultiplier = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealingStatics().OutgoingHealingMultiplierDef,
	                                                           EvaluateParameters,
	                                                           SourceOutgoingHealingMultiplier);
    
	float TargetIncomingHealingMultiplier = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealingStatics().IncomingHealingMultiplierDef,
	                                                           EvaluateParameters,
	                                                           TargetIncomingHealingMultiplier);
	if (AssetTags.HasTag(FECRGameplayTags::Get().GameplayEffect_HealingIgnoresMultipliers))
	{
		TargetIncomingHealingMultiplier = 1.0f;
		SourceOutgoingHealingMultiplier = 1.0f;
	}

	const float AttenuatedHealing = FMath::Max(
		0, BaseHealing * SourceOutgoingHealingMultiplier * TargetIncomingHealingMultiplier);

    UE_LOG(LogTemp, Warning, TEXT("Res healing %f"), AttenuatedHealing)

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(UECRHealthSet::GetHealingAttribute(), EGameplayModOp::Additive,
		                               AttenuatedHealing));

#endif // #if WITH_SERVER_CODE
}
