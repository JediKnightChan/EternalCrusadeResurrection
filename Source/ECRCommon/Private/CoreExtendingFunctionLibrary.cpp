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

int64 UCoreExtendingFunctionLibrary::DateTimeToUnixTimestamp(FDateTime DateTime)
{
	return DateTime.ToUnixTimestamp();
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
	}

	// Getting [0, 1] random fraction
	const FDateTime DateTime = FDateTime::Now();
	const FRandomStream RandomStream{DateTime.GetMillisecond()};
	const float Fraction = RandomStream.GetFraction();

	for (int i = 0; i < CumBreaks.Num() - 1; i++)
	{
		if (CumBreaks[i] <= Fraction && Fraction < CumBreaks[i + 1])
		{
			return Names[i];
		}
	}

	// Shouldn't get here, but for IDE warning to disappear :)
	return NAME_None;
}

double UCoreExtendingFunctionLibrary::DegreesToStandardized(double Degrees)
{
	Degrees = UKismetMathLibrary::GenericPercent_FloatFloat(Degrees, 360.0f);
	// Holy Emperor knows how this works, but we are definitely in [-360, 360]

	if (Degrees > 180.0f)
	{
		// We got into (180, 360], will go to (-180, 0]
		return Degrees - 360.0f;
	}
	if (Degrees < -180.0f)
	{
		// We got into [-360, -180), will go to [0, 180)
		return Degrees + 360.0f;
	}
	// We got into [-180, 180]
	return Degrees;
}


double UCoreExtendingFunctionLibrary::GetRuntimeFloatCurveValue(const FRuntimeFloatCurve& InCurve, double InTime)
{
	return InCurve.GetRichCurveConst()->Eval(InTime);
}


void UCoreExtendingFunctionLibrary::GetPawnAimOffsetDifference(APawn* Pawn, double& PitchDiff, double& YawDiff)
{
	if (!Pawn)
	{
		PitchDiff = 0;
		YawDiff = 0;
	}

	const FRotator BaseAimRotation = Pawn->GetControlRotation();
	const FRotator ActorRotation = Pawn->GetActorRotation();

	PitchDiff = DegreesToStandardized(BaseAimRotation.Pitch - ActorRotation.Pitch);
	YawDiff = DegreesToStandardized(BaseAimRotation.Yaw - ActorRotation.Yaw);
}
