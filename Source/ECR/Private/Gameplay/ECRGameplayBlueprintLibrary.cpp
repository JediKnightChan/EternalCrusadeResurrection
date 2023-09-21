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
