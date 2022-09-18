#include "ECREditor/Public/ECREditor.h"
#include "ECREditor/Public/MeshCustomizationUICustomization.h"
#include "ECRCommon/Public/CustomizationSaverComponent.h"
#include "PropertyEditor/Public/PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FECREditorModule"

void FECREditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	// to register our custom property
	PropertyModule.RegisterCustomClassLayout(
		UCustomizationSaverComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FMeshCustomizationUICustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FECREditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		// unregister properties when the module is shutdown
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(UCustomizationSaverComponent::StaticClass()->GetFName());

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FECREditorModule, ECREditor)
