#include "ECREditor/Public/MeshCustomizationUICustomization.h"

#include "DetailCustomizations/Private/ActorComponentDetails.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Components/SlateWrapperTypes.h"
#include "ECRCommon/Public/CustomizationSaverComponent.h"
#include "ECRCommon/Public/ModularMeshSaverComponent.h"
#include "Internationalization/Internationalization.h"
#include "Engine/EngineBaseTypes.h"
#include "Modules/ModuleManager.h"
#include "Misc/MessageDialog.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"


#define LOCTEXT_NAMESPACE "MeshCustomizationDetails"

TSharedRef<IDetailCustomization> FMeshCustomizationUICustomization::MakeInstance()
{
	return MakeShareable(new FMeshCustomizationUICustomization);
}

void FMeshCustomizationUICustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& EditCategory = DetailLayout.EditCategory(
		"Save", LOCTEXT("SaveCategoryName", "Save Settings"));
	
	EditCategory.AddCustomRow(LOCTEXT("MeshCustomizationActionsSaveRow", "Actions"), false)
	            .NameContent()
		[
			SNew(SButton)
		.ContentPadding(2)
		.OnClicked(this, &FMeshCustomizationUICustomization::ApplyChanges, &DetailLayout)
			[
				SNew(STextBlock)
			.Text(LOCTEXT("MeshCustomizationActionsApply", "Apply Changes"))
			.ToolTipText(LOCTEXT("MeshCustomizationActionsApply_ToolTip",
			                     "Apply customization in this blueprint."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		]
		.ValueContent()
		[
			SNew(SButton)
		.ContentPadding(2)
		.OnClicked(this, &FMeshCustomizationUICustomization::SaveAssets, &DetailLayout)
			[
				SNew(STextBlock)
			.Text(LOCTEXT("MeshCustomizationActionsSave", "Save"))
			.ToolTipText(LOCTEXT("MeshCustomizationActionsSave_ToolTip",
			                     "Save configuration into the specified directory and file."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
			]

		];
}

TArray<UCustomizationSaverComponent*> FMeshCustomizationUICustomization::GetInstancesBeingCustomized(
	const IDetailLayoutBuilder* DetailLayout) const
{
	// Result
	TArray<UCustomizationSaverComponent*> ResultSelectedObjects;
	
	// Looping through customized objects weak pointers, getting existing objects of right type
	
	// (UCustomizationSaverComponent)
	TArray<TWeakObjectPtr<UObject>> WeakSelectedObjects = DetailLayout->GetSelectedObjects();
	

	for (TWeakObjectPtr<UObject>& WeakObjectBeingCustomized : WeakSelectedObjects)
	{
		if (UObject* ObjectBeingCustomized = WeakObjectBeingCustomized.Get())
		{
			if (UCustomizationSaverComponent* Component = Cast<UCustomizationSaverComponent>(ObjectBeingCustomized))
			{
				ResultSelectedObjects.AddUnique(Component);
			}
		}
	}
	return ResultSelectedObjects;
}

FReply FMeshCustomizationUICustomization::SaveAssets(IDetailLayoutBuilder* DetailLayout) const
{
	TArray<UCustomizationSaverComponent*> InstancesBeingCustomized = GetInstancesBeingCustomized(DetailLayout);
	for (UCustomizationSaverComponent* CustomizationComponent : InstancesBeingCustomized)
	{
		CustomizationComponent->SaveToAsset();
	}
	return FReply::Handled();
}

FReply FMeshCustomizationUICustomization::ApplyChanges(IDetailLayoutBuilder* DetailLayout) const
{
	UCustomizationSaverComponent* DefaultActor = Cast<UCustomizationSaverComponent>(UCustomizationSaverComponent::StaticClass()->GetDefaultObject(true));
	DefaultActor->ApplyChanges();
	
	TArray<UCustomizationSaverComponent*> InstancesBeingCustomized = GetInstancesBeingCustomized(DetailLayout);
	for (UCustomizationSaverComponent* CustomizationComponent : InstancesBeingCustomized)
	{
		CustomizationComponent->ApplyChanges();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
