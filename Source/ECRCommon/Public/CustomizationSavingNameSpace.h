// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CustomizationSavingNameSpace.generated.h"


UCLASS( ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent) )
class ECRCOMMON_API UCustomizationSavingNameSpace : public USceneComponent
{
	GENERATED_BODY()
public:	
	// Sets default values for this component's properties
	UCustomizationSavingNameSpace();

	/* Save destination root directory for customization assets (eg, /Game/Characters/SpaceMarine/Customization/) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString SaveDestinationRootDirectory;
};
