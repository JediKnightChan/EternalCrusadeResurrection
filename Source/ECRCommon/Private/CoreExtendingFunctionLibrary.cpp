// Copyleft: All rights reversed


#include "CoreExtendingFunctionLibrary.h"

#include "Kismet/KismetMathLibrary.h"


double UCoreExtendingFunctionLibrary::GetCurrentTimeInSeconds()
{
	const FDateTime CurrentTime = UKismetMathLibrary::UtcNow();
	const double SecondsSinceUnix = static_cast<double>(CurrentTime.ToUnixTimestamp());
	const int32 Millisecond = CurrentTime.GetMillisecond();
	const double Result = SecondsSinceUnix + Millisecond / 1000.0f;
	return Result;
}


TMap<UObject*, FString> UCoreExtendingFunctionLibrary::SortUObjectToStringMap(TMap<UObject*, FString> MapToSort)
{
	MapToSort.ValueSort([](const FString& A, const FString& B)
	{
		// Sort strings alphabetically
		return A.Compare(B) < 0;
	});
	return MapToSort;
}


TMap<UObject*, int32> UCoreExtendingFunctionLibrary::SortUObjectToIntMap(TMap<UObject*, int32> MapToSort)
{
	MapToSort.ValueSort([](const int32& A, const int32& B)
	{
		// Sort numbers
		return A < B;
	});
	return MapToSort;
}

int32 UCoreExtendingFunctionLibrary::LeastCommonMultiple(TArray<int32> NumberArray)
{
	int32 LeastCommonMultiple = 1;
	for (const int32 Number : NumberArray)
	{
		LeastCommonMultiple = FMath::LeastCommonMultiplier(Number, LeastCommonMultiple);
	}
	return LeastCommonMultiple;
}

FName UCoreExtendingFunctionLibrary::GetRandomName(TMap<FName, float> NamesToWeights)
{
	TArray<FName> Names;
	TArray<float> CumBreaks = {0};
	float CumSum = 0.f;

	// Creating mapping from names to cumulative weights
	for (auto [Name, Weight] : NamesToWeights)
	{
		UE_LOG(LogTemp, Warning, TEXT("Name is %s, weight is %f, cumbreak is %f"), *(Name.ToString()), Weight,
		       CumSum + Weight);
		Names.Add(Name);
		CumBreaks.Add(CumSum + Weight);
		CumSum += Weight;
	}

	// Normalizing cumulative breaks to [0, 1] for mapping to random fraction
	for (float& CumBreak : CumBreaks)
	{
		if (CumSum != 0)
		{
			CumBreak = CumBreak / CumSum;
		}
		UE_LOG(LogTemp, Warning, TEXT("Norm cumbreak is %f, sum was %f"), CumBreak, CumSum);
	}

	// Getting [0, 1] random fraction
	const FDateTime DateTime = FDateTime::Now();
	const FRandomStream RandomStream{DateTime.GetMillisecond()};
	const float Fraction = RandomStream.GetFraction();

	UE_LOG(LogTemp, Warning, TEXT("Names num is %d"), Names.Num());
	UE_LOG(LogTemp, Warning, TEXT("Cumbreaks num is %d"), CumBreaks.Num());

	for (int i = 0; i < CumBreaks.Num() - 1; i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("Factions is %f, left is %f, right is %f"), Fraction, CumBreaks[i],
		       CumBreaks[i+1]);
		if (CumBreaks[i] <= Fraction && Fraction < CumBreaks[i + 1])
		{
			return Names[i];
		}
	}

	// Shouldn't get here, but for IDE warning to disappear :)
	return NAME_None;
}
