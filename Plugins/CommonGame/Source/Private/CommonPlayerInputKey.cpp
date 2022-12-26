// Copyright Epic Games, Inc. All Rights Reserved.

#include "CommonPlayerInputKey.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "CommonInputSubsystem.h"
#include "CommonTextBlock.h"
#include "CommonUISettings.h"
#include "CommonUITypes.h"
#include "Styling/SlateBrush.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "CommonPlayerController.h"
#include "CommonLocalPlayer.h"

#define LOCTEXT_NAMESPACE "CommonKeybindWidget"

DECLARE_LOG_CATEGORY_EXTERN(LogCommonPlayerInput, Log, All);
DEFINE_LOG_CATEGORY(LogCommonPlayerInput);

struct FSlateDrawUtil
{
	static void DrawBrushCenterFit(
		FSlateWindowElementList& ElementList,
		uint32 InLayer,
		const FGeometry& InAllottedGeometry,
		const FSlateBrush* InBrush,
		const FLinearColor& InTint = FLinearColor::White)
	{
		DrawBrushCenterFitWithOffset
		(
			ElementList,
			InLayer,
			InAllottedGeometry,
			InBrush,
			InTint,
			FVector2D(0, 0)
		);
	}

	static void DrawBrushCenterFitWithOffset(
		FSlateWindowElementList& ElementList,
		uint32 InLayer,
		const FGeometry& InAllottedGeometry,
		const FSlateBrush* InBrush,
		const FLinearColor& InTint,
		const FVector2D InOffset)
	{
		if (!InBrush)
		{
			return;
		}

		const FVector2D AreaSize = InAllottedGeometry.GetLocalSize();
		const FVector2D ProgressSize = InBrush->GetImageSize();
		const float FitScale = FMath::Min(FMath::Min(AreaSize.X / ProgressSize.X, AreaSize.Y / ProgressSize.Y), 1.0f);
		const FVector2D FinalSize = FitScale * ProgressSize;

		const FVector2D Offset = (InAllottedGeometry.GetLocalSize() * 0.5f) - (FinalSize * 0.5f) + InOffset;

		FSlateDrawElement::MakeBox
		(
			ElementList,
			InLayer,
			InAllottedGeometry.ToPaintGeometry(Offset, FinalSize),
			InBrush,
			ESlateDrawEffect::None,
			InTint
		);
	}
};



void FMeasuredText::SetText(const FText& InText)
{
	CachedText = InText;
	bTextDirty = true;
}

FVector2D FMeasuredText::UpdateTextSize(const FSlateFontInfo &InFontInfo, float FontScale) const
{
	if (bTextDirty)
	{
		bTextDirty = false;
		CachedTextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(CachedText, InFontInfo, FontScale);
	}

	return CachedTextSize;
}

UCommonPlayerInputKey::UCommonPlayerInputKey(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, BoundKeyFallback(EKeys::Invalid)
	, InputTypeOverride(ECommonInputType::Count)
{
	FrameSize = FVector2D(0, 0);
}

void UCommonPlayerInputKey::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateKeybindWidget();

	if (IsDesignTime())
	{
		ShowHoldBackPlate();
		RecalculateDesiredSize();
	}
}

void UCommonPlayerInputKey::NativeConstruct()
{
	Super::NativeConstruct();
}

void UCommonPlayerInputKey::NativeDestruct()
{
	if (ProgressPercentageMID)
	{
		// Need to restore the material on the brush before we kill off the MID.
		HoldProgressBrush.SetResourceObject(ProgressPercentageMID->GetMaterial());

		ProgressPercentageMID->MarkAsGarbage();
		ProgressPercentageMID = nullptr;
	}

	Super::NativeDestruct();
}

int32 UCommonPlayerInputKey::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (bDrawProgress)
	{
		FSlateDrawUtil::DrawBrushCenterFit
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry,
			&HoldProgressBrush,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint() * HoldProgressBrush.GetTint(InWidgetStyle))
		);
	}

	if (bDrawCountdownText)
	{
		const FVector2D CountdownTextOffset = (AllottedGeometry.GetLocalSize() - CountdownText.GetTextSize()) * 0.5f;

		FSlateDrawElement::MakeText
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry.ToOffsetPaintGeometry(CountdownTextOffset),
			CountdownText.GetText(),
			CountdownTextFont,
			ESlateDrawEffect::None,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint())
		);
	}
	else if (bDrawBrushForKey)
	{
		// Draw Shadow
		FSlateDrawUtil::DrawBrushCenterFitWithOffset
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry,
			&CachedKeyBrush,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint() * FLinearColor::Black),
			FVector2D(1, 1)
		);

		FSlateDrawUtil::DrawBrushCenterFit
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry,
			&CachedKeyBrush,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint() * CachedKeyBrush.GetTint(InWidgetStyle))
		);
	}
	else if (KeybindText.GetTextSize().X > 0)
	{
		const FVector2D FrameOffset = (AllottedGeometry.GetLocalSize() - FrameSize) * 0.5f;

		FSlateDrawElement::MakeBox
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry.ToPaintGeometry(FrameOffset, FrameSize),
			&KeyBindTextBorder,
			ESlateDrawEffect::None,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint() * KeyBindTextBorder.GetTint(InWidgetStyle))
		);

		const FVector2D ActionTextOffset = (AllottedGeometry.GetLocalSize() - KeybindText.GetTextSize()) * 0.5f;

		FSlateDrawElement::MakeText
		(
			OutDrawElements,
			++MaxLayer,
			AllottedGeometry.ToOffsetPaintGeometry(ActionTextOffset),
			KeybindText.GetText(),
			KeyBindTextFont,
			ESlateDrawEffect::None,
			FLinearColor(InWidgetStyle.GetColorAndOpacityTint())
		);
	}

	return MaxLayer;
}

void UCommonPlayerInputKey::StartHoldProgress(FName HoldActionName, float HoldDuration)
{
	if (HoldActionName == BoundAction && ensureMsgf(HoldDuration > 0.0f, TEXT("Trying to perform hold action \"%s\" with no HoldDuration"), *BoundAction.ToString()))
	{
		HoldKeybindDuration = HoldDuration;
		HoldKeybindStartTime = GetWorld()->GetRealTimeSeconds();

		UpdateHoldProgress();
	}
}

void UCommonPlayerInputKey::StopHoldProgress(FName HoldActionName, bool bCompletedSuccessfully)
{
	if (HoldActionName == BoundAction)
	{
		HoldKeybindStartTime = 0.f;
		HoldKeybindDuration = 0.f;

		if (ensure(ProgressPercentageMID))
		{
			ProgressPercentageMID->SetScalarParameterValue(PercentageMaterialParameterName, 0.f);
		}

		if (bDrawCountdownText)
		{
			bDrawCountdownText = false;
			Invalidate(EInvalidateWidget::Paint);
			RecalculateDesiredSize();
		}
	}
}

void UCommonPlayerInputKey::SyncHoldProgress()
{
	// If we had an active hold action, stop it
	if (HoldKeybindStartTime > 0.f)
	{
		StopHoldProgress(BoundAction, false);
	}
}

void UCommonPlayerInputKey::UpdateHoldProgress()
{
	if (HoldKeybindStartTime != 0.f && HoldKeybindDuration > 0.f)
	{
		const float CurrentTime = GetWorld()->GetRealTimeSeconds();
		const float ElapsedTime = FMath::Min(CurrentTime - HoldKeybindStartTime, HoldKeybindDuration);
		const float RemainingTime = FMath::Max(0.0f, HoldKeybindDuration - ElapsedTime);

		if (ElapsedTime < HoldKeybindDuration && ensure(ProgressPercentageMID))
		{
			const float HoldKeybindPercentage = ElapsedTime / HoldKeybindDuration;
			ProgressPercentageMID->SetScalarParameterValue(PercentageMaterialParameterName, HoldKeybindPercentage);

			// Schedule a callback for next tick to update the hold progress again.
			GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::UpdateHoldProgress);		
		}

		if (bShowTimeCountDown)
		{
			FNumberFormattingOptions Options;
			Options.MinimumFractionalDigits = 1;
			Options.MaximumFractionalDigits = 1;
			CountdownText.SetText(FText::AsNumber(RemainingTime, &Options));

			bDrawCountdownText = true;
			Invalidate(EInvalidateWidget::Paint);
			RecalculateDesiredSize();
		}
	}
}

void UCommonPlayerInputKey::UpdateKeybindWidget()
{
	if (!GetOwningPlayer<ACommonPlayerController>())
	{
		bWaitingForPlayerController = true;
		return;
	}

	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();

	if (CommonInputSubsystem && !CommonInputSubsystem->ShouldShowInputKeys())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const bool bIsUsingGamepad = (InputTypeOverride == ECommonInputType::Gamepad) || ((CommonInputSubsystem != nullptr) && (CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)) ;

	if (!BoundKey.IsValid())
	{
		BoundKey = BoundKeyFallback;
	}
	UE_LOG(LogCommonPlayerInput, Verbose, TEXT("UCommonKeybindWidget::UpdateKeybindWidget: Action: %s Key: %s"), *(BoundAction.ToString()), *(BoundKey.ToString()));

	// Must be called before Update, due to the creation of ProgressPercentageMID which will be used in Update
	SetupHoldKeybind();

	bool NewDrawBrushForKey = false;
	bool NeedToRecalcSize = false;

	if (BoundKey.IsValid())
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);

		ShowHoldBackPlate();

		NeedToRecalcSize = true;
	}
	else
	{
		if (bShowUnboundStatus)
		{
			SetVisibility(ESlateVisibility::HitTestInvisible);
			NewDrawBrushForKey = false;

			KeybindText.SetText(LOCTEXT("Unbound", "Unbound"));

			NeedToRecalcSize = true;
		}
		else
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (bDrawBrushForKey != NewDrawBrushForKey)
	{
		bDrawBrushForKey = NewDrawBrushForKey;
		Invalidate(EInvalidateWidget::Paint);
	}

	// As RecalculateDesiredSize relies on the bDrawBrushForKey 
	// we shouldn't call it until that value has been finalized
	// for the update
	if (NeedToRecalcSize)
	{
		RecalculateDesiredSize();
	}
}

void UCommonPlayerInputKey::SetBoundKey(FKey NewKey)
{
	if (NewKey != BoundKey)
	{
		BoundKeyFallback = NewKey;
		BoundAction = NAME_None;
		UpdateKeybindWidget();
	}
}

void UCommonPlayerInputKey::SetBoundAction(FName NewBoundAction)
{
	bool bUpdateWidget = true;

	if (BoundAction != NewBoundAction)
	{
		BoundAction = NewBoundAction;
	}

	if (bUpdateWidget)
	{
		UpdateKeybindWidget();
	}
}

void UCommonPlayerInputKey::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UCommonLocalPlayer* CommonLocalPlayer = GetOwningLocalPlayer<UCommonLocalPlayer>())
	{
		CommonLocalPlayer->OnPlayerControllerSet.AddUObject(this, &ThisClass::HandlePlayerControllerSet);
	}
}

void UCommonPlayerInputKey::SetForcedHoldKeybind(bool InForcedHoldKeybind)
{
	if (InForcedHoldKeybind)
	{
		SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus::ForcedHold);
	}
	else
	{
		SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus::NoForcedHold);
	}
}

void UCommonPlayerInputKey::SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus InForcedHoldKeybindStatus)
{
	ForcedHoldKeybindStatus = InForcedHoldKeybindStatus;

	UpdateKeybindWidget();
}

void UCommonPlayerInputKey::SetShowProgressCountDown(bool bShow)
{
	bShowTimeCountDown = bShow;
}

void UCommonPlayerInputKey::SetupHoldKeybind()
{
	ACommonPlayerController* OwningCommonPlayer = Cast<ACommonPlayerController>(GetOwningPlayer());

	// Setup the hold
	if (ForcedHoldKeybindStatus == ECommonKeybindForcedHoldStatus::ForcedHold)
	{
		bIsHoldKeybind = true;
	}
	else if (ForcedHoldKeybindStatus == ECommonKeybindForcedHoldStatus::NeverShowHold)
	{
		bIsHoldKeybind = false;
	}

	if (ensure(OwningCommonPlayer))
	{
		if (bIsHoldKeybind)
		{
			// Setup the ProgressPercentageMID
			if (ProgressPercentageMID == nullptr)
			{
				if (UMaterialInterface* Material = Cast<UMaterialInterface>(HoldProgressBrush.GetResourceObject()))
				{
					ProgressPercentageMID = UMaterialInstanceDynamic::Create(Material, this);
					HoldProgressBrush.SetResourceObject(ProgressPercentageMID);
				}
			}
			SyncHoldProgress();
		}
	}
}

void UCommonPlayerInputKey::ShowHoldBackPlate()
{
	bool bDirty = false;

	if (IsHoldKeybind())
	{
		float BrushSizeAsValue = 32.0f;
		
		float DesiredBoxSize = BrushSizeAsValue + 10.0f;
		if (!bDrawBrushForKey)
		{
			DesiredBoxSize += 14.0f;
		}

		const FVector2D NewDesiredBrushSize(DesiredBoxSize, DesiredBoxSize);
		if (HoldProgressBrush.GetImageSize() != NewDesiredBrushSize)
		{
			HoldProgressBrush.SetImageSize(NewDesiredBrushSize);
			bDirty = true;
		}

		if (!bDrawProgress)
		{
			bDrawProgress = true;
			bDirty = true;
		}

		static const FName BackAlphaName = TEXT("BackAlpha");
		static const FName OutlineAlphaName = TEXT("OutlineAlpha");

		if (ProgressPercentageMID)
		{
			ProgressPercentageMID->SetScalarParameterValue(BackAlphaName, 0.2f);
			ProgressPercentageMID->SetScalarParameterValue(OutlineAlphaName, 0.4f);
		}
	}
	else
	{
		if (bDrawProgress)
		{
			bDrawProgress = false;
			bDirty = true;
		}
	}

	if (bDirty)
	{
		Invalidate(EInvalidateWidget::Paint);
	}
}

void UCommonPlayerInputKey::HandlePlayerControllerSet(UCommonLocalPlayer* LocalPlayer, APlayerController* PlayerController)
{
	if (bWaitingForPlayerController && GetOwningPlayer<ACommonPlayerController>())
	{
		UpdateKeybindWidget();
		bWaitingForPlayerController = false;
	}
}

void UCommonPlayerInputKey::RecalculateDesiredSize()
{
	FVector2D MaximumDesiredSize(0, 0);
	float LayoutScale = 1;

	if (bDrawProgress)
	{
		MaximumDesiredSize = FVector2D::Max(MaximumDesiredSize, HoldProgressBrush.GetImageSize());
	}

	if (bDrawCountdownText)
	{
		MaximumDesiredSize = FVector2D::Max(MaximumDesiredSize, CountdownText.UpdateTextSize(CountdownTextFont, LayoutScale));
	}
	else if (bDrawBrushForKey)
	{
		MaximumDesiredSize = FVector2D::Max(MaximumDesiredSize, CachedKeyBrush.GetImageSize());
	}
	else
	{
		const FVector2D KeybindTextSize = KeybindText.UpdateTextSize(KeyBindTextFont, LayoutScale);
		FrameSize = FVector2D::Max(KeybindTextSize, KeybindFrameMinimumSize) + KeybindTextPadding.GetDesiredSize();
		MaximumDesiredSize = FVector2D::Max(MaximumDesiredSize, FrameSize);
	}

	SetMinimumDesiredSize(MaximumDesiredSize);
}

#undef LOCTEXT_NAMESPACE
