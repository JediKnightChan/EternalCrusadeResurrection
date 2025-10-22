#include "System/ECRNavigationConfig.h"


FECRNavigationConfig::FECRNavigationConfig()
{
	KeyEventRules.Emplace(EKeys::A, EUINavigation::Left);
	KeyEventRules.Emplace(EKeys::D, EUINavigation::Right);
	KeyEventRules.Emplace(EKeys::W, EUINavigation::Up);
	KeyEventRules.Emplace(EKeys::S, EUINavigation::Down);
}
