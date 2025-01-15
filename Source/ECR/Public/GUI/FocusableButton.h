#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "FocusableButton.generated.h"

/**
 * Custom UButton that uses a custom SButton for additional focus logic
 */
UCLASS()
class ECR_API UFocusableButton : public UButton
{
    GENERATED_UCLASS_BODY()

protected:
    // Override to provide a custom Slate widget
    virtual TSharedRef<SWidget> RebuildWidget() override;
};