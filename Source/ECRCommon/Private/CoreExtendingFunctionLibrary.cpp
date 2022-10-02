// Copyleft: All rights reversed


#include "CoreExtendingFunctionLibrary.h"


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
