// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "AsyncAction_ShowConfirmation.generated.h"

class UUserWidget;
class UCommonGameDialogDescriptor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCommonMessagingResultMCDelegate, ECommonMessagingResult, Result);

/**
 * Allows easily triggering an async confirmation dialog in blueprints that you can then wait on the result.
 */
UCLASS()
class UAsyncAction_ShowConfirmation : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (BlueprintInternalUseOnly = "true", WorldContext = "InWorldContextObject"))
	static UAsyncAction_ShowConfirmation* ShowConfirmationYesNo(
		UObject* InWorldContextObject, FText Title, FText Message
	);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (BlueprintInternalUseOnly = "true", WorldContext = "InWorldContextObject"))
	static UAsyncAction_ShowConfirmation* ShowConfirmationOkCancel(
		UObject* InWorldContextObject, FText Title, FText Message
	);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, meta = (BlueprintInternalUseOnly = "true", WorldContext = "InWorldContextObject"))
	static UAsyncAction_ShowConfirmation* ShowConfirmationCustom(
		UObject* InWorldContextObject, UCommonGameDialogDescriptor* Descriptor
	);

	virtual void Activate() override;

public:
	UPROPERTY(BlueprintAssignable)
	FCommonMessagingResultMCDelegate OnResult;

private:
	void HandleConfirmationResult(ECommonMessagingResult ConfirmationResult);

	UPROPERTY(Transient)
	UObject* WorldContextObject;

	UPROPERTY(Transient)
	ULocalPlayer* TargetLocalPlayer;

	UPROPERTY(Transient)
	UCommonGameDialogDescriptor* Descriptor;
};
