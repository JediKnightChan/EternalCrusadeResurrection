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
	
	// Sorting

	/** Sort UObjects according to their string parameter exposed as value in map - without changing original map */
	UFUNCTION(BlueprintCallable)
	static TMap<UObject*, FString> SortUObjectToStringMap(TMap<UObject*, FString> MapToSort);

	/** Sort UObjects according to their integer parameter exposed as value in map - without changing original map */
	UFUNCTION(BlueprintCallable)
	static TMap<UObject*, int32> SortUObjectToIntMap(TMap<UObject*, int32> MapToSort);

	// Math

	/** Get least common multiple of an array of numbers */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int32 LeastCommonMultiple(TArray<int32> NumberArray);

	/** Get random name item according to weights */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static FName GetRandomName(TMap<FName, float> NamesToWeights);

	/** Get current UTC time in seconds (unix timestamp) */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static double GetCurrentTimeInSeconds();
};
