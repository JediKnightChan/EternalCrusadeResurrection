// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ECRVerbMessage.h"
#include "GameplayEffectTypes.h"

#include "ECRVerbMessageHelpers.generated.h"

class APlayerState;
class APlayerController;
struct FGameplayCueParameters;


UCLASS()
class ECR_API UECRVerbMessageHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ECR")
	static APlayerState* GetPlayerStateFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "ECR")
	static APlayerController* GetPlayerControllerFromObject(UObject* Object);

	UFUNCTION(BlueprintCallable, Category = "ECR")
	static FGameplayCueParameters VerbMessageToCueParameters(const FECRVerbMessage& Message);

	UFUNCTION(BlueprintCallable, Category = "ECR")
	static FECRVerbMessage CueParametersToVerbMessage(const FGameplayCueParameters& Params);
};
