#pragma once

#include "DetailCustomizations/Private/ActorDetails.h"
#include "ECRCommon/Public/CustomizationSaverComponent.h"
#include "PropertyEditor/Public/IDetailCustomization.h"

class FMeshCustomizationUICustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** Customizes interface, adding buttons for SaveAssets and ApplyChanges */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	TArray<UCustomizationSaverComponent*> GetInstancesBeingCustomized(const IDetailLayoutBuilder* DetailLayout) const;

	/** Calls save assets on CustomizationSaverComponent customized instances */
	FReply SaveAssets(IDetailLayoutBuilder* DetailLayout) const;

	/** Calls apply changes on CustomizationSaverComponent customized instances */
	FReply ApplyChanges(IDetailLayoutBuilder* DetailLayout) const;
};
