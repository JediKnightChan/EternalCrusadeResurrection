// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintEncryptionBPLibrary.generated.h"

/* 
*	Function library class for encryption.
*/
UCLASS()
class UBlueprintEncryptionBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
	/** Encrypts string using key. */
	UFUNCTION(BlueprintCallable, Category = "ECR|Encryption")
	static FString EncryptString(FString InputString, FString Key);

	/** Decrypts string using key. */
	UFUNCTION(BlueprintCallable, Category = "ECR|Encryption")
	static FString DecryptString(FString InputString, FString Key);
};
