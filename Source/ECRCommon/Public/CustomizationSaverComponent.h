// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CustomizationSaverComponent.generated.h"


UCLASS(ClassGroup=(Customization), meta=(BlueprintSpawnableComponent, IsBlueprintBase="true"))
class ECRCOMMON_API UCustomizationSaverComponent : public USceneComponent
{
	GENERATED_BODY()

protected:
	/** Directory to save into */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Save, meta = (AllowPrivateAccess = "true"))
	FString SaveDestinationDirectory;

	/** File to save into */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Save, meta = (AllowPrivateAccess = "true"))
	FString SaveDestinationFilename;
public:
	// Sets default values for this component's properties
	UCustomizationSaverComponent();

	/** Save customization parameters to the UDataAsset */
	virtual void SaveToAsset();

	/** Save customization parameters to the UDataAsset */
	virtual void ApplyChanges();
	
	/** Return full save path based on directory and filename */
	FString GetFullSavePath();
};
