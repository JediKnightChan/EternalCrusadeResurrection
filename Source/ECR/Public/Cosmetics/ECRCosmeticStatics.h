// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "ECRCosmeticStatics.generated.h"

/**  */
UCLASS()
class UECRCosmeticStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static UECRPawnComponent_CharacterParts* GetPawnCustomizationComponentFromActor(AActor* TargetActor);

	static void AddMontageToLoadQueueIfNeeded(const TSoftObjectPtr<UAnimMontage>& Montage,
	                                          TArray<FSoftObjectPath>& MontagesToLoad);
};
