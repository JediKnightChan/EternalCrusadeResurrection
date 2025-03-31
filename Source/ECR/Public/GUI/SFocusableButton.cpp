#include "SFocusableButton.h"

FReply SFocusableButton::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
    SetHover(true);
    return SButton::OnFocusReceived(MyGeometry, InFocusEvent);
}

void SFocusableButton::OnFocusLost(const FFocusEvent& InFocusEvent)
{
    SetHover(false);
    TAttribute<bool> EmptyAttr;
    SetHover(EmptyAttr);
    SButton::OnFocusLost(InFocusEvent);
}
