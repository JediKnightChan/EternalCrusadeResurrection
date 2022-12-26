// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CommonPlayerInputKey.generated.h"

class UCommonLocalPlayer;
class APlayerController;

UENUM(BlueprintType)
enum class ECommonKeybindForcedHoldStatus : uint8
{
	NoForcedHold,
	ForcedHold,
	NeverShowHold
};

USTRUCT()
struct FMeasuredText
{
	GENERATED_BODY()

public:
	FText GetText() const { return CachedText; }
	void SetText(const FText& InText);

	FVector2D GetTextSize() const { return CachedTextSize; }
	FVector2D UpdateTextSize(const FSlateFontInfo &InFontInfo, float FontScale = 1.0f) const;

private:

	FText CachedText;
	mutable FVector2D CachedTextSize;
	mutable bool bTextDirty = true;
};

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class COMMONGAME_API UCommonPlayerInputKey : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonPlayerInputKey(const FObjectInitializer& ObjectInitializer);

	/** Update the key and associated display based on our current Boundaction */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void UpdateKeybindWidget();

	/** Set the bound key for our keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetBoundKey(FKey NewBoundAction);

	/** Set the bound action for our keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetBoundAction(FName NewBoundAction);

	/** Force this keybind to be a hold keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget", meta=(DeprecatedFunction, DeprecationMessage = "Use SetForcedHoldKeybindStatus instead"))
	void SetForcedHoldKeybind(bool InForcedHoldKeybind);

	/** Force this keybind to be a hold keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetForcedHoldKeybindStatus(ECommonKeybindForcedHoldStatus InForcedHoldKeybindStatus);

	/** Force this keybind to be a hold keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetShowProgressCountDown(bool bShow);

	/** Set the axis scale value for this keybind */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetAxisScale(const float NewValue) { AxisScale = NewValue; }

	/** Set the preset name override value for this keybind. */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	void SetPresetNameOverride(const FName NewValue) { PresetNameOverride = NewValue; }

	/** Our current BoundAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keybind Widget")
	FName BoundAction;

	/** Scale to read when using an axis Mapping */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keybind Widget")
	float AxisScale;

	/** Key this widget is bound to set directly in blueprint. Used when we want to reference a specific key instead of an action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keybind Widget")
	FKey BoundKeyFallback;

	/** Allows us to set the input type explicitly for the keybind widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind Widget")
	ECommonInputType InputTypeOverride;

	/** Allows us to set the preset name explicitly for the keybind widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keybind Widget")
	FName PresetNameOverride;

	/** Setting that can show this keybind as a hold or never show it as a hold (even if it is) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Keybind Widget")
	ECommonKeybindForcedHoldStatus ForcedHoldKeybindStatus;

	/** Called through a delegate when we start hold progress */
	UFUNCTION()
	void StartHoldProgress(FName HoldActionName, float HoldDuration);

	/** Called through a delegate when we stop hold progress */
	UFUNCTION()
	void StopHoldProgress(FName HoldActionName, bool bCompletedSuccessfully);

	/** Get whether this keybind is a hold action. */
	UFUNCTION(BlueprintCallable, Category = "Keybind Widget")
	bool IsHoldKeybind() const { return bIsHoldKeybind; }

	UFUNCTION()
	bool IsBoundKeyValid() const { return BoundKey.IsValid(); }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	void RecalculateDesiredSize();

	/** Overridden to destroy our MID */
	virtual void NativeDestruct() override;

	/** Whether or not this keybind widget is currently set to be a hold keybind */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Keybind Widget", meta=(ScriptName = "IsHoldKeybindValue"))
	bool bIsHoldKeybind;

	/**  */
	UPROPERTY(Transient)
	bool bShowKeybindBorder;

	UPROPERTY(Transient)
	FVector2D FrameSize;

	UPROPERTY(BlueprintReadOnly, Category = "Keybind Widget")
	bool bShowTimeCountDown;

	/** Derived Key this widget is bound to */
	UPROPERTY(BlueprintReadOnly, Category = "Keybind Widget")
	FKey BoundKey;

	/** Material for showing Progress */
	UPROPERTY(EditDefaultsOnly, Category = "Keybind Widget")
	FSlateBrush HoldProgressBrush;

	/** The key bind text border. */
	UPROPERTY(EditDefaultsOnly, Category = "Keybind Widget")
	FSlateBrush KeyBindTextBorder;

	/** Should this keybinding widget display information that it is currently unbound? */
	UPROPERTY(EditAnywhere, Category = "Keybind Widget")
	bool bShowUnboundStatus = false;

	/** The font to apply at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Font")
	FSlateFontInfo KeyBindTextFont;

	/** The font to apply at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Font")
	FSlateFontInfo CountdownTextFont;

	UPROPERTY(Transient)
	FMeasuredText CountdownText;

	UPROPERTY(Transient)
	FMeasuredText KeybindText;

	UPROPERTY(Transient)
	FMargin KeybindTextPadding;

	UPROPERTY(Transient)
	FVector2D KeybindFrameMinimumSize;

	/** The material parameter name for hold percentage in the HoldKeybindImage */
	UPROPERTY(EditDefaultsOnly, Category = "Keybind Widget")
	FName PercentageMaterialParameterName;	

	/** MID for the progress percentage */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ProgressPercentageMID;

	virtual void NativeOnInitialized() override;

private:
	/**
	 * Synchronizes the hold progress to whatever is currently set in the
	 * owning player controller.
	 */
	void SyncHoldProgress();

	/** Called for updating the HoldKeybindImage during a hold keybind */
	void UpdateHoldProgress();

	/** Called when we want to set up this keybind widget as a hold keybind */
	void SetupHoldKeybind();

	void ShowHoldBackPlate();

	void HandlePlayerControllerSet(UCommonLocalPlayer* LocalPlayer, APlayerController* PlayerController);

	/** Time when we started using a hold keybind */
	float HoldKeybindStartTime = 0;

	/** How long, in seconds, we will be doing a hold keybind */
	float HoldKeybindDuration = 0;

	bool bDrawProgress = false;
	bool bDrawBrushForKey = false;
	bool bDrawCountdownText = false;
	bool bWaitingForPlayerController = false;

	UPROPERTY(Transient)
	FSlateBrush CachedKeyBrush;
};
