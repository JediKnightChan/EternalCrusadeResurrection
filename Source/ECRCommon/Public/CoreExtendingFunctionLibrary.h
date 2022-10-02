// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreExtendingFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCoreExtendingFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
    
    /** Sort UObjects according to their string parameter exposed as value in map - without changing original map */
	UFUNCTION(BlueprintCallable)
	static TMap<UObject*, FString> SortUObjectToStringMap(TMap<UObject*, FString> MapToSort);
	
	/** Sort UObjects according to their integer parameter exposed as value in map - without changing original map */
	UFUNCTION(BlueprintCallable)
	static TMap<UObject*, int32> SortUObjectToIntMap(TMap<UObject*, int32> MapToSort);
};
