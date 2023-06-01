// Copyright Epic Games, Inc. All Rights Reserved.

#include "GUI/Weapons/SHitMarkerConfirmationWidget.h"
#include "Gameplay/Weapons/ECRWeaponStateComponent.h"

SHitMarkerConfirmationWidget::SHitMarkerConfirmationWidget()
{
}

void SHitMarkerConfirmationWidget::Construct(const FArguments& InArgs, const FLocalPlayerContext& InContext,
                                             const TMap<FGameplayTag, FSlateBrush>& ZoneOverrideImages)
{
	PerHitMarkerImage = InArgs._PerHitMarkerImage;
	PerHitMarkerZoneOverrideImages = ZoneOverrideImages;
	AllyHitMarkerImage = InArgs._AllyHitMarkerImage;
	AnyHitsMarkerImage = InArgs._AnyHitsMarkerImage;
	bColorAndOpacitySet = InArgs._ColorAndOpacity.IsSet();
	ColorAndOpacity = InArgs._ColorAndOpacity;

	MyContext = InContext;
}

int32 SHitMarkerConfirmationWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                                            const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                                            int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FVector2D LocalCenter = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));

	const bool bDrawMarkers = (HitNotifyOpacity > KINDA_SMALL_NUMBER);

	if (bDrawMarkers)
	{
		// Check if we should use screen-space damage location hit notifies
		TArray<FECRScreenSpaceHitLocation> LastWeaponDamageScreenLocations;
		if (APlayerController* PC = MyContext.IsInitialized() ? MyContext.GetPlayerController() : nullptr)
		{
			if (UECRWeaponStateComponent* WeaponStateComponent = PC->FindComponentByClass<UECRWeaponStateComponent>())
			{
				WeaponStateComponent->GetLastWeaponDamageScreenLocations(/*out*/ LastWeaponDamageScreenLocations);
			}
		}

		bool bDealtFriendlyDamage = false;
		bool bDealtEnemyDamage = false;

		if (LastWeaponDamageScreenLocations.Num() > 0)
		{
			for (const FECRScreenSpaceHitLocation& Hit : LastWeaponDamageScreenLocations)
			{
				const FSlateBrush* LocationMarkerImage = nullptr;

				switch (Hit.HitSuccess)
				{
				case Enemy:
					bDealtEnemyDamage = true;
					LocationMarkerImage = PerHitMarkerZoneOverrideImages.Find(Hit.HitZone);
					if (LocationMarkerImage == nullptr)
					{
						LocationMarkerImage = PerHitMarkerImage;
					}
					break;
				case Ally:
					bDealtFriendlyDamage = true;
					LocationMarkerImage = AllyHitMarkerImage;
					break;
				default:
					break;
				}

				if (LocationMarkerImage == nullptr)
				{
					UE_LOG(LogTemp, Warning, TEXT("Loc marker empty"))
					continue;
				}

				FLinearColor MarkerColor = bColorAndOpacitySet
					                           ? ColorAndOpacity.Get().GetColor(InWidgetStyle)
					                           : (InWidgetStyle.GetColorAndOpacityTint() * LocationMarkerImage->GetTint(
						                           InWidgetStyle));
				MarkerColor.A *= HitNotifyOpacity;

				const FVector2D WindowSSLocation = Hit.Location + MyCullingRect.GetTopLeft();
				// Accounting for window trim when not in fullscreen mode
				const FSlateRenderTransform DrawPos(AllottedGeometry.AbsoluteToLocal(WindowSSLocation));

				const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
					LocationMarkerImage->ImageSize, FSlateLayoutTransform(-(LocationMarkerImage->ImageSize * 0.5f)),
					DrawPos));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, LocationMarkerImage, DrawEffects,
				                           MarkerColor);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Any hits %d, Enemy damage %d friendly %d"),
		       AnyHitsMarkerImage != nullptr ? 1: 0,
		       bDealtEnemyDamage ? 1 : 0,
		       bDealtFriendlyDamage ? 1: 0)
		if (AnyHitsMarkerImage != nullptr && !bDealtFriendlyDamage && bDealtEnemyDamage)
		{
			FLinearColor MarkerColor = bColorAndOpacitySet
				                           ? ColorAndOpacity.Get().GetColor(InWidgetStyle)
				                           : (InWidgetStyle.GetColorAndOpacityTint() * AnyHitsMarkerImage->GetTint(
					                           InWidgetStyle));
			MarkerColor.A *= HitNotifyOpacity;

			// Otherwise show the hit notify in the center of the reticle
			const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
				AnyHitsMarkerImage->ImageSize,
				FSlateLayoutTransform(LocalCenter - (AnyHitsMarkerImage->ImageSize * 0.5f))));
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, AnyHitsMarkerImage, DrawEffects,
			                           MarkerColor);
		}
	}

	return LayerId;
}

FVector2D SHitMarkerConfirmationWidget::ComputeDesiredSize(float) const
{
	return FVector2D(100.0f, 100.0f);
}

void SHitMarkerConfirmationWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime,
                                        const float InDeltaTime)
{
	HitNotifyOpacity = 0.0f;

	if (APlayerController* PC = MyContext.IsInitialized() ? MyContext.GetPlayerController() : nullptr)
	{
		if (UECRWeaponStateComponent* DamageMarkerComponent = PC->FindComponentByClass<UECRWeaponStateComponent>())
		{
			const double TimeSinceLastHitNotification = DamageMarkerComponent->GetTimeSinceLastHitNotification();
			if (TimeSinceLastHitNotification < HitNotifyDuration)
			{
				HitNotifyOpacity = FMath::Clamp(1.0f - (float)(TimeSinceLastHitNotification / HitNotifyDuration), 0.0f,
				                                1.0f);
			}
		}
	}
}
