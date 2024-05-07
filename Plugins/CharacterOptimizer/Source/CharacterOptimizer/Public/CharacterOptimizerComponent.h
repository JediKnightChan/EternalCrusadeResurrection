// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"

#include "CharacterOptimizerComponent.generated.h"

USTRUCT(BlueprintType)
struct FCharOptimizationSettings
{
	GENERATED_BODY()

	FCharOptimizationSettings()
	{
		CharMoveComponentTickInterval = 0.0f;
		CharTickInterval = 0.0f;
		bShadowsTurnedOn = true;
	}

	FCharOptimizationSettings(double NewCharMoveComponentTickInterval, double NewCharTickInterval,
	                          bool bNewShadowsTurnedOn)
	{
		CharMoveComponentTickInterval = NewCharMoveComponentTickInterval;
		CharTickInterval = NewCharTickInterval;
		bShadowsTurnedOn = bNewShadowsTurnedOn;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPreserveRatio="true"))
	double CharMoveComponentTickInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPreserveRatio="true"))
	double CharTickInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPreserveRatio="true"))
	bool bShadowsTurnedOn;
};

/**
 * 
 */
UCLASS(ClassGroup=(CharacterOptimization), meta=(BlueprintSpawnableComponent))
class CHARACTEROPTIMIZER_API UCharacterOptimizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterOptimizerComponent();

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnRegister() override;

private:
	/** Whether to apply optimization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bOptimizationEnabled;

	/** If char is not in viewport and further than this distance, we'll apply last wave settings to him */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	double NotInViewportDistance;
	
	/** Optimization settings applied to char if he is closer than FirstWaveDistance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FCharOptimizationSettings ZeroWaveSettings;

	/** Distance for the first wave of optimization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	double FirstWaveDistance;

	/** Optimization settings applied to char if he is between FirstWaveDistance and SecondWaveDistance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FCharOptimizationSettings FirstWaveSettings;

	/** Distance for the second wave of optimization */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	double SecondWaveDistance;

	/** Optimization settings applied to char if he is further than SecondWaveDistance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FCharOptimizationSettings SecondWaveSettings;

protected:
	void ApplyOptimizationSettingsToChar(FCharOptimizationSettings& Settings);

private:
	UPROPERTY()
	ACharacter* Char;
};
