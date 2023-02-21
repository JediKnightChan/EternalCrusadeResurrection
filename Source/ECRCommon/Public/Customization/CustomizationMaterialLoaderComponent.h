// Copyleft: All rights reversed

#pragma once

#include "CustomizationMaterialAsset.h"
#include "CustomizationMaterialLoaderComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(ModularCustomizationCommon), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationMaterialLoaderComponent : public USceneComponent
{
	GENERATED_BODY()

	/** Whether to load customization on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bLoadOnBeginPlay;

	/** Material customization configs we want to load */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<UCustomizationMaterialAsset*> MaterialConfigs;

public:
	UCustomizationMaterialLoaderComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void LoadMaterialCustomization(TArray<UCustomizationMaterialAsset*> NewConfigs);
};
