// Copyleft: All rights reversed


#include "RegexFunctionLibrary.h"


bool URegexFunctionLibrary::CheckRegexMatch(const FString Regex, const FString Input)
{
	const FRegexPattern RegexPattern{Regex};
	FRegexMatcher RegexMatcher{RegexPattern, Input};
	return RegexMatcher.FindNext();
}
