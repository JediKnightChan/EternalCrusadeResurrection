// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ECRInventoryItemDefinition.generated.h"

class UECRInventoryItemInstance;

//////////////////////////////////////////////////////////////////////

// Represents a fragment of an item definition
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class UECRInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UECRInventoryItemInstance* Instance) const
	{
	}
};

//////////////////////////////////////////////////////////////////////

/**
 * UECRInventoryItemDefinition
 */
UCLASS(Blueprintable, Const, Abstract)
class UECRInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UECRInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FName QuickBarChannelName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UECRInventoryItemFragment>> Fragments;

public:
	const UECRInventoryItemFragment* FindFragmentByClass(TSubclassOf<UECRInventoryItemFragment> FragmentClass) const;
};


UCLASS()
class UECRInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UECRInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UECRInventoryItemDefinition> ItemDef,
	                                                                   TSubclassOf<UECRInventoryItemFragment>
	                                                                   FragmentClass);
};
