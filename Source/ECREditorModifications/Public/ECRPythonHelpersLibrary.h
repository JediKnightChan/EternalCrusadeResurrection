#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "ECRPythonHelpersLibrary.generated.h"

UCLASS()
class ECREDITORMODIFICATIONS_API UECRPythonHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameplayTags", meta = (ScriptMethod))
	static FGameplayTag ConvertStringToGameplayTag(const FString& TagName);
};
