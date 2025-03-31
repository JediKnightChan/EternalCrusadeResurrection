#include "ECRPythonHelpersLibrary.h"

#include "Misc/FileHelper.h"


FGameplayTag UECRPythonHelpersLibrary::ConvertStringToGameplayTag(UObject* AnyObject, FString TagName)
{
	// Get the GameplayTagsManager instance
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName));

	// Optional: Check if the tag exists
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid GameplayTag: %s"), *TagName);
	}

	return Tag;
}

void UECRPythonHelpersLibrary::ExportDataTableAsCsv(UDataTable* DataTable, FString OutputPath)
{
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExportDataTableAsCsv: DataTable is null"));
		return;
	}


	// Check if the output path is valid
	const FString AbsolutePath = FPaths::ConvertRelativePathToFull(OutputPath);
	if (!FPaths::ValidatePath(AbsolutePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("ExportDataTableAsCsv: Invalid output path"));
		return;
	}

	// Get row names and data structure for CSV export
	FString CSVContent = DataTable->GetTableAsCSV();

	// Write the CSV content to file
	if (FFileHelper::SaveStringToFile(CSVContent, *AbsolutePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Log, TEXT("DataTable exported successfully to: %s"), *AbsolutePath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to export DataTable to: %s"), *AbsolutePath);
	}
}


UClass* UECRPythonHelpersLibrary::GetParentClass(UBlueprint* Blueprint)
{
	return Blueprint ? Blueprint->ParentClass : nullptr;
}