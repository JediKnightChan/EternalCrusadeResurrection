// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "UObject/ObjectMacros.h"
#include "Styling/SlateBrush.h"
#include "Components/Widget.h"
#include "GameplayTagContainer.h"

class SHitMarkerConfirmationWidget;

#include "HitMarkerConfirmationWidget.generated.h"

UCLASS()
class UHitMarkerConfirmationWidget : public UWidget
{
	GENERATED_BODY()

public:
	UHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer);

	//~UWidget interface
	protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~End of UWidget interface

	//~UVisual interface
	public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~End of UVisual interface
	
	/** The duration (in seconds) to display hit notifies (they fade to transparent over this time)  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(ClampMin=0.0, ForceUnits=s))
	float HitNotifyDuration = 0.4f;
	
	/** The marker image to draw for individual hit markers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush PerHitMarkerImage;

	/** The marker image to draw for individual hit markers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateBrush AllyHitMarkerImage;

	/** Map from zone tag (e.g., weak spot) to override marker images for individual location hits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TMap<FGameplayTag, FSlateBrush> PerHitMarkerZoneOverrideImages;

	/** The marker image to draw if there are any hits at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	FSlateBrush AnyHitsMarkerImage;

private:
	/** Internal slate widget representing the actual marker visuals */
	TSharedPtr<SHitMarkerConfirmationWidget> MyMarkerWidget;
};
