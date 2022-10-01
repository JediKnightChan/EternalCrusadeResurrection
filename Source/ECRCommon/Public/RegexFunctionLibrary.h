// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RegexFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API URegexFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/** Check whether string matches regular expression */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool CheckRegexMatch(FString Regex, FString Input);
};
