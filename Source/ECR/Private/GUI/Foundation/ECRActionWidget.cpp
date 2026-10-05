// Copyright Epic Games, Inc. All Rights Reserved.

#include "GUI/Foundation/ECRActionWidget.h"

#include "CommonInputSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Settings/ECRSettingsLocal.h"

FSlateBrush UECRActionWidget::GetIcon() const
{
	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();
	if (!CommonInputSubsystem)
	{
		return Super::GetIcon();
	}

	FSlateBrush SlateBrush;

	FName GamepadName = NAME_None;
	if (UECRSettingsLocal* Settings = UECRSettingsLocal::Get())
	{
		GamepadName = Settings->GamepadPlatform;
	}

	if (GamepadName == NAME_None)
	{
		GamepadName = CommonInputSubsystem->GetCurrentGamepadName();
	}

	if (AssociatedInputAction)
	{
		if (const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = GetEnhancedInputSubsystem())
		{
			TArray<FKey> BoundKeys = EnhancedInputSubsystem->QueryKeysMappedToAction(AssociatedInputAction);
			BoundKeys = BoundKeys.FilterByPredicate([CommonInputSubsystem](FKey Key)
			{
				return Key.IsValid() && Key.IsGamepadKey() == (CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad);
			});

			if (!BoundKeys.IsEmpty() && UCommonInputPlatformSettings::Get()->TryGetInputBrush(SlateBrush, BoundKeys[0], CommonInputSubsystem->GetCurrentInputType(), GamepadName))
			{
				return SlateBrush;
			}
		}
	} else
	{
		if (UCommonInputPlatformSettings::Get()->TryGetInputBrush(SlateBrush, AssociatedKey,
															  CommonInputSubsystem->GetCurrentInputType(), GamepadName))
		{
			return SlateBrush;
		}
	}
	
	return Super::GetIcon();
}

void UECRActionWidget::EnforceUpdate()
{
	UpdateActionWidget();
}

UEnhancedInputLocalPlayerSubsystem* UECRActionWidget::GetEnhancedInputSubsystem() const
{
	const UWidget* BoundWidget = DisplayedBindingHandle.GetBoundWidget();
	const ULocalPlayer* BindingOwner = BoundWidget ? BoundWidget->GetOwningLocalPlayer() : GetOwningLocalPlayer();

	return BindingOwner->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}
