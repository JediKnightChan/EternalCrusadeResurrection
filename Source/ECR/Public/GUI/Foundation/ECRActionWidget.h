// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonActionWidget.h"
#include "ECRActionWidget.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;

/** An action widget that will get the icon of key that is currently assigned to the common input action on this widget */
UCLASS(BlueprintType, Blueprintable)
class UECRActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:

	//~ Begin UCommonActionWidget interface
	virtual FSlateBrush GetIcon() const override;
	//~ End of UCommonActionWidget interface

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey AssociatedKey;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UInputAction> AssociatedInputAction;

	UFUNCTION(BlueprintCallable)
	void EnforceUpdate();
private:

	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
	
};