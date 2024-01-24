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

public:
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

	/** Convert datetime to unix */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static int64 DateTimeToUnixTimestamp(FDateTime DateTime);

	/** Converts degrees to [-180, 180] range */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static double DegreesToStandardized(double Degrees);

	/** Evaluates the value of a Runtime Float Curve using the given time. */
	UFUNCTION(BlueprintPure, Category = "Math|RuntimeFloatCurve")
	static double GetRuntimeFloatCurveValue(const FRuntimeFloatCurve& InCurve, double InTime);

	// UE Utils

	/** Legacy, possibly broken: returns Pitch difference and Yaw difference for pawn (in [-180, 180] range) */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static void GetPawnAimOffsetDifference(APawn* Pawn, double& PitchDiff, double& YawDiff);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool IsActorOfClass(AActor* Actor, TSubclassOf<AActor> Class);
};
