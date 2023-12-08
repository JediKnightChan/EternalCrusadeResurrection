// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ECRCosmeticAnimationTypes.h"
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

	// Choose the best actor given the tags
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TSubclassOf<AActor> SelectBestActor(const FECRActorSelectionSet Set,
	                                           const FGameplayTagContainer CosmeticTags);

	// Choose the best montage given the tags
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TSoftObjectPtr<UAnimMontage> SelectBestMontage(const FECRAnimMontageSelectionSet Set,
	                                                      FGameplayTagContainer CosmeticTags);

	// Choose the best mesh given the tags
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static USkeletalMesh* SelectBestMesh(const FECRMeshSelectionSet Set,
	                                     FGameplayTagContainer CosmeticTags);

	// Choose the best anim instance given the tags
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static TSubclassOf<UAnimInstance> SelectBestAnimInstance(const FECRAnimInstanceSelectionSet Set,
	                                             FGameplayTagContainer CosmeticTags);
};
