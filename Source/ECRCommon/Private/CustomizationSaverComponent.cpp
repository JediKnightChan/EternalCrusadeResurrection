// Copyleft: All rights reversed


#include "ECRCommon/Public/CustomizationSaverComponent.h"

#include "AssetSelection.h"

// Sets default values for this component's properties
UCustomizationSaverComponent::UCustomizationSaverComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}

void UCustomizationSaverComponent::SaveToAsset()
{
}

void UCustomizationSaverComponent::ApplyChanges()
{
}

FString UCustomizationSaverComponent::GetFullSavePath()
{
	// Handle case when directory doesn't end with slash
	const FString SaveDestinationPackagePath =
		SaveDestinationDirectory +
		(SaveDestinationDirectory.EndsWith("/")
			 ? SaveDestinationFilename
			 : "/" + SaveDestinationFilename);
	
	return SaveDestinationPackagePath;
}
