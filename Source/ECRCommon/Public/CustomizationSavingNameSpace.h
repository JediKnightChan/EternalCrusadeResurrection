// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CustomizationSavingNameSpace.generated.h"


UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationSavingNameSpace : public USceneComponent
{
	GENERATED_BODY()

	/** Save every child CustomizationElementaryModule, overwriting / skipping it if it already exists,
	 * and produce CustomizationLoaderAsset */
	void SaveLoadout(bool bDoOverwrite);
public:
	// Sets default values for this component's properties
	UCustomizationSavingNameSpace();

	/* Save destination root directory for customization assets (eg, /Game/Characters/SpaceMarine/Customization/) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString SaveDestinationRootDirectory;

	/** Save every child CustomizationElementaryModule, overwriting if it already exists,
	 * and produce CustomizationLoaderAsset */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveLoadoutOverwritingExistingModules();

	/** Save every child CustomizationElementaryModule, skipping if it already exists,
	 * and produce CustomizationLoaderAsset */
	UFUNCTION(CallInEditor, BlueprintCallable)
	void SaveLoadoutSkippingExistingModules();
};
