// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintGraph/Classes/K2Node_AsyncAction.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "K2Node_AsyncAction_ListenForGameplayMessages.generated.h"

class FBlueprintActionDatabaseRegistrar;

/**
 * Blueprint node which is spawned to handle the async logic for UAsyncAction_RegisterGameplayMessageReceiver
 */

UCLASS()
class UK2Node_AsyncAction_ListenForGameplayMessages : public UK2Node_AsyncAction
{
	GENERATED_BODY()

	//~UEdGraphNode interface
	virtual void PostReconstructNode() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* ChangedPin) override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	//~End of UEdGraphNode interface
	
	//~UK2Node interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void AllocateDefaultPins() override;
	//~End of UK2Node interface

protected:
	virtual bool HandleDelegates(
		const TArray<FBaseAsyncTaskHelper::FOutputPinAndLocalVariable>& VariableOutputs, UEdGraphPin* ProxyObjectPin,
		UEdGraphPin*& InOutLastThenPin, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext) override;

private:
	
	// Add the GetPayload flow to the end of the delegate handler's logic chain
	bool HandlePayloadImplementation(
		FMulticastDelegateProperty* CurrentProperty,
		const FBaseAsyncTaskHelper::FOutputPinAndLocalVariable& ProxyObjectVar,
		const FBaseAsyncTaskHelper::FOutputPinAndLocalVariable& PayloadVar,
		const FBaseAsyncTaskHelper::FOutputPinAndLocalVariable& ActualChannelVar,
		UEdGraphPin*& InOutLastActivatedThenPin, UEdGraph* SourceGraph, FKismetCompilerContext& CompilerContext);

	// Make sure the output Payload wildcard matches the input PayloadType 
	void RefreshOutputPayloadType();

	UEdGraphPin* GetPayloadPin() const;
	UEdGraphPin* GetPayloadTypePin() const;
	UEdGraphPin* GetOutputChannelPin() const;
};
