// Copyleft: All rights reversed


#include "Gameplay/ECRGameplayBlueprintLibrary.h"

double UECRGameplayBlueprintLibrary::CalculateDamageAttenuationForArmorPenetration(double ArmorPenetration,
                                                                                   const double Toughness, const double Armor)
{
	// Make sure Toughness - ArmorPenetration >= 0 or ArmorPenetration <= Toughness
	ArmorPenetration = FMath::Min(ArmorPenetration, Toughness);

	// Main formula
	double Attenuation = 1 - 2 * (1 / (1 + FMath::Exp(-0.015 * (Toughness - ArmorPenetration))) - 0.5);

	// If Armor > ArmorPenetration, then no damage at all
	if (ArmorPenetration < Armor)
	{
		Attenuation = 0.0f;
	}

	return Attenuation;
}

FVector UECRGameplayBlueprintLibrary::VRandConeNormalDistribution(const FVector& Dir, const float ConeHalfAngleRad,
	const float Exponent)
{
	if (ConeHalfAngleRad > 0.f)
	{
		const float ConeHalfAngleDegrees = FMath::RadiansToDegrees(ConeHalfAngleRad);

		// consider the cone a concatenation of two rotations. one "away" from the center line, and another "around" the circle
		// apply the exponent to the away-from-center rotation. a larger exponent will cluster points more tightly around the center
		const float FromCenter = FMath::Pow(FMath::FRand(), Exponent);
		const float AngleFromCenter = FromCenter * ConeHalfAngleDegrees;
		const float AngleAround = FMath::FRand() * 360.0f;

		FRotator Rot = Dir.Rotation();
		FQuat DirQuat(Rot);
		FQuat FromCenterQuat(FRotator(0.0f, AngleFromCenter, 0.0f));
		FQuat AroundQuat(FRotator(0.0f, 0.0, AngleAround));
		FQuat FinalDirectionQuat = DirQuat * AroundQuat * FromCenterQuat;
		FinalDirectionQuat.Normalize();

		return FinalDirectionQuat.RotateVector(FVector::ForwardVector);
	}
	else
	{
		return Dir.GetSafeNormal();
	}
}
