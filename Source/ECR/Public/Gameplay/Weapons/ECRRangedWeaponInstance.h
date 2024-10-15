// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"

#include "ECRWeaponInstance.h"

#include "ECRRangedWeaponInstance.generated.h"

class UPhysicalMaterial;

/**
 * UECRRangedWeaponInstance
 *
 * A piece of equipment representing a ranged weapon spawned and applied to a pawn
 */
UCLASS()
class UECRRangedWeaponInstance : public UECRWeaponInstance
{
	GENERATED_BODY()

public:
	UECRRangedWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	void UpdateDebugVisualization();
#endif

	int32 GetBulletsPerCartridge() const
	{
		return BulletsPerCartridge;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure)
	/** Returns the current spread angle (in degrees, diametrical) */
	float GetCalculatedSpreadAngle() const
	{
		return CurrentSpreadAngle;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetCalculatedSpreadAngleMultiplier() const
	{
		return bHasFirstShotAccuracy ? 0.0f : CurrentSpreadAngleMultiplier;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetCurrentHeat() const
	{
		return CurrentHeat;
	}

	bool HasFirstShotAccuracy() const
	{
		return bHasFirstShotAccuracy;
	}

	float GetSpreadExponent() const
	{
		return SpreadExponent;
	}

	float GetMaxDamageRange() const
	{
		return MaxDamageRange;
	}

	float GetBulletTraceSweepRadius() const
	{
		return BulletTraceSweepRadius;
	}

protected:
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spread|Fire Params", meta=(AllowPrivateAccess="true"))
	float Debug_MinHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spread|Fire Params", meta=(AllowPrivateAccess="true"))
	float Debug_MaxHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spread|Fire Params",
		meta=(ForceUnits=deg, AllowPrivateAccess="true"))
	float Debug_MinSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spread|Fire Params",
		meta=(ForceUnits=deg, AllowPrivateAccess="true"))
	float Debug_MaxSpreadAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spread Debugging", meta=(AllowPrivateAccess="true"))
	float Debug_CurrentHeat = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spread Debugging",
		meta = (ForceUnits=deg, AllowPrivateAccess="true"))
	float Debug_CurrentSpreadAngle = 0.0f;

	// The current *combined* spread angle multiplier
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spread Debugging",
		meta=(ForceUnits=x, AllowPrivateAccess="true"))
	float Debug_CurrentSpreadAngleMultiplier = 1.0f;

#endif

	// Spread exponent, affects how tightly shots will cluster around the center line
	// when the weapon has spread (non-perfect accuracy). Higher values will cause shots
	// to be closer to the center (default is 1.0 which means uniformly within the spread range)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=0.1), Category="Spread|Fire Params")
	float SpreadExponent = 1.0f;

	// A curve that maps the heat to the spread angle
	// The X range of this curve typically sets the min/max heat range of the weapon
	// The Y range of this curve is used to define the min and maximum spread angle
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spread|Fire Params")
	FRuntimeFloatCurve HeatToSpreadCurve;

	// A curve that maps the current heat to the amount a single shot will further 'heat up'
	// This is typically a flat curve with a single data point indicating how much heat a shot adds,
	// but can be other shapes to do things like punish overheating by adding progressively more heat.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToHeatPerShotCurve;

	// A curve that maps the current heat to the heat cooldown rate per second
	// This is typically a flat curve with a single data point indicating how fast the heat
	// wears off, but can be other shapes to do things like punish overheating by slowing down
	// recovery at high heat.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spread|Fire Params")
	FRuntimeFloatCurve HeatToCoolDownPerSecondCurve;

	// Time since firing before spread cooldown recovery begins (in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Fire Params", meta=(ForceUnits=s))
	float SpreadRecoveryCooldownDelay = 0.0f;

	// Should the weapon have perfect accuracy when both player and weapon spread are at their minimum value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Fire Params")
	bool bAllowFirstShotAccuracy = false;

	// Multiplier for heat when getting spread based on current heat
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spread|Fire Params", meta=(ForceUnits=x))
	float HeatToSpreadMappingMultiplier = 1.0f;

	// Multiplier when adding heat per shot
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spread|Fire Params", meta=(ForceUnits=x))
	float HeatPerShotMultiplier = 1.0f;
	
	// Multiplier applied by modifiers
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Modifier = 1.0f;

	// Multiplier when in an aiming camera mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Aiming = 1.0f;

	// Multiplier when in bracing mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Bracing = 1.0f;

	// Multiplier when standing still or moving very slowly
	// (starts to fade out at StandingStillSpeedThreshold, and is gone completely by StandingStillSpeedThreshold + StandingStillToMovingSpeedRange)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_StandingStill = 1.0f;

	// Rate at which we transition to/from the standing still accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params")
	float TransitionRate_StandingStill = 5.0f;

	// Speeds at or below this are considered standing still
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillSpeedThreshold = 80.0f;

	// Speeds no more than this above StandingStillSpeedThreshold are used to feather down the standing still bonus until it's back to 1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits="cm/s"))
	float StandingStillToMovingSpeedRange = 20.0f;


	// Multiplier when crouching, smoothly blended to based on TransitionRate_Crouching
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Crouching = 1.0f;

	// Rate at which we transition to/from the crouching accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params")
	float TransitionRate_Crouching = 5.0f;

	// Minimum spread multiplier possible
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Min = 0.2f;

	// Maximum spread multiplier possible
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_Max = 5.0f;

	// Spread multiplier while jumping/falling, smoothly blended to based on TransitionRate_JumpingOrFalling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params", meta=(ForceUnits=x))
	float SpreadAngleMultiplier_JumpingOrFalling = 1.0f;

	// Rate at which we transition to/from the jumping/falling accuracy (higher values are faster, though zero is instant; @see FInterpTo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spread|Player Params")
	float TransitionRate_JumpingOrFalling = 5.0f;

	// Number of bullets to fire in a single cartridge (typically 1, but may be more for shotguns)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config")
	int32 BulletsPerCartridge = 1;

	// The maximum distance at which this weapon can deal damage
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config", meta=(ForceUnits=cm))
	float MaxDamageRange = 25000.0f;

	// The radius for bullet traces sweep spheres (0.0 will result in a line trace)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config", meta=(ForceUnits=cm))
	float BulletTraceSweepRadius = 0.0f;

	// Distance on which damage is done with distance multiplier 1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config", meta=(ForceUnits=cm))
	float DamageNearDistance = 1000.0f;

	// Distance on which damage is done with distance multiplier equal to DamageFarMultiplier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config", meta=(ForceUnits=cm))
	float DamageFarDistance = 10000.0f;

	// Multiplier of damage for distance equal to DamageFarDistance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config")
	float DamageFarMultiplier = 1.0f;

	// Time between shots
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config")
	float AutoShotsInterval = 0.2f;

	// Reload time when there is no ammo in magazine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config")
	float EmptyReloadTime = 0.0f;

	// Reload time when there is ammo in magazine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config")
	float NonEmptyReloadTime = 0.0f;

	// List of special tags that affect how damage is dealt
	// These tags will be compared to tags in the physical material of the thing being hit
	// If more than one tag is present, the multipliers will be combined multiplicatively
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Config")
	TMap<FGameplayTag, float> MaterialDamageMultiplier;

	// Whether want to raise weapon before shooting (for heavy weapons)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Config")
	bool bWantWeaponUp = false;

	// Duration multiplier for raising weapon before shooting (for montage playing)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Config", meta=(ForceUnits=x))
	float WeaponUpDurationMultiplier = 1.0f;
private:
	// The current heat
	UPROPERTY(Replicated)
	float CurrentHeat = 0.0f;

	// The current spread angle (in degrees, diametrical)
	float CurrentSpreadAngle = 0.0f;

	// Do we currently have first shot accuracy?
	bool bHasFirstShotAccuracy = false;

	// The current *combined* spread angle multiplier
	float CurrentSpreadAngleMultiplier = 1.0f;

	// The current standing still multiplier
	float StandingStillMultiplier = 1.0f;

	// The current jumping/falling multiplier
	float JumpFallMultiplier = 1.0f;

	// The current crouching multiplier
	float CrouchingMultiplier = 1.0f;

public:
	void Tick(float DeltaSeconds);

	//~UECREquipmentInstance interface
	virtual void OnEquipped();
	virtual void OnUnequipped();
	//~End of UECREquipmentInstance interface

	/** Add heat as 1 shot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddSpread();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void RemoveHeat(float DeltaHeat);

	//~IECRAbilitySourceInterface interface
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr,
	                                     const FGameplayTagContainer* TargetTags = nullptr) const override;
	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial,
	                                             const FGameplayTagContainer* SourceTags = nullptr,
	                                             const FGameplayTagContainer* TargetTags = nullptr) const override;
	//~End of IECRAbilitySourceInterface interface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	void ComputeSpreadRange(float& MinSpread, float& MaxSpread);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	void ComputeHeatRange(float& MinHeat, float& MaxHeat);

	inline float ClampHeat(float NewHeat)
	{
		float MinHeat;
		float MaxHeat;
		ComputeHeatRange(/*out*/ MinHeat, /*out*/ MaxHeat);

		return FMath::Clamp(NewHeat, MinHeat, MaxHeat);
	}

	// Updates the spread and returns true if the spread is at minimum
	bool UpdateSpread(float DeltaSeconds);

	// Updates the multipliers and returns true if they are at minimum
	bool UpdateMultipliers(float DeltaSeconds);
};
