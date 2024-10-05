#include "ECRPythonHelpersLibrary.h"

FGameplayTag UECRPythonHelpersLibrary::ConvertStringToGameplayTag(const FString& TagName)
{
	// Get the GameplayTagsManager instance
	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName));

	// Optional: Check if the tag exists
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid GameplayTag: %s"), *TagName);
	}

	return Tag;
}
