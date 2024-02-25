// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonUserTypes.h"
#include "OnlineError.h"

void FOnlineResultInformation::FromOnlineError(const FOnlineErrorType& InOnlineError)
{
#if COMMONUSER_OSSV1
	bWasSuccessful = InOnlineError.WasSuccessful();
	ErrorId = InOnlineError.GetErrorCode();
	ErrorText = InOnlineError.GetErrorMessage();
#else
	bWasSuccessful = InOnlineError != UE::Online::Errors::Success();
	ErrorId = InOnlineError.GetErrorId();
	ErrorText = InOnlineError.GetText();
#endif
}
