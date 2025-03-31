#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"

/**
 * Custom SButton to add focus-based hover logic
 */
class ECR_API SFocusableButton : public SButton
{
public:
    // Override to handle keyboard focus
    virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
    virtual void OnFocusLost(const FFocusEvent& InFocusEvent) override;
};