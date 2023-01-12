// Copyright Epic Games, Inc. All Rights Reserved.

#include "GUI/IndicatorSystem/IndicatorDescriptor.h"
#include "Engine/LocalPlayer.h"


bool FIndicatorProjection::Project(const UIndicatorDescriptor& IndicatorDescriptor, const FSceneViewProjectionData& InProjectionData, const FVector2D& ScreenSize, FVector& OutScreenPositionWithDepth)
{
	if (USceneComponent* Component = IndicatorDescriptor.GetSceneComponent())
	{
		TOptional<FVector> WorldLocation;
		if (IndicatorDescriptor.GetComponentSocketName() != NAME_None)
		{
			WorldLocation = Component->GetSocketTransform(IndicatorDescriptor.GetComponentSocketName()).GetLocation();
		}
		else
		{
			WorldLocation = Component->GetComponentLocation();
		}

		const FVector ProjectWorldLocation = WorldLocation.GetValue() + IndicatorDescriptor.GetWorldPositionOffset();
		const EActorCanvasProjectionMode ProjectionMode = IndicatorDescriptor.GetProjectionMode();
		
		switch (ProjectionMode)
		{
			case EActorCanvasProjectionMode::ComponentPoint:
			{
				if (WorldLocation.IsSet())
				{
					FVector2D OutScreenSpacePosition;
					if (ULocalPlayer::GetPixelPoint(InProjectionData, ProjectWorldLocation, OutScreenSpacePosition, &ScreenSize))
					{
						OutScreenSpacePosition += IndicatorDescriptor.GetScreenSpaceOffset();

						OutScreenPositionWithDepth = FVector(OutScreenSpacePosition.X, OutScreenSpacePosition.Y, FVector::Dist(InProjectionData.ViewOrigin, ProjectWorldLocation));
						return true;
					}
				}

				return false;
			}
			case EActorCanvasProjectionMode::ComponentScreenBoundingBox:
			case EActorCanvasProjectionMode::ActorScreenBoundingBox:
			{
				FBox IndicatorBox;
				if (ProjectionMode == EActorCanvasProjectionMode::ActorScreenBoundingBox)
				{
					IndicatorBox = Component->GetOwner()->GetComponentsBoundingBox();
				}
				else
				{
					IndicatorBox = Component->Bounds.GetBox();
				}

				FVector2D LL, UR;
				if (ULocalPlayer::GetPixelBoundingBox(InProjectionData, IndicatorBox, LL, UR, &ScreenSize))
				{
					const FVector& BoundingBoxAnchor = IndicatorDescriptor.GetBoundingBoxAnchor();
					const FVector2D& ScreenSpaceOffset = IndicatorDescriptor.GetScreenSpaceOffset();
					
					OutScreenPositionWithDepth.X = FMath::Lerp(LL.X, UR.X, BoundingBoxAnchor.X) + ScreenSpaceOffset.X;
					OutScreenPositionWithDepth.Y = FMath::Lerp(LL.Y, UR.Y, BoundingBoxAnchor.Y) + ScreenSpaceOffset.Y;
					OutScreenPositionWithDepth.Z = FVector::Dist(InProjectionData.ViewOrigin, ProjectWorldLocation);
					return true;
				}

				return false;
			}
			case EActorCanvasProjectionMode::ActorBoundingBox:
			case EActorCanvasProjectionMode::ComponentBoundingBox:
			{
				FBox IndicatorBox;
				if (ProjectionMode == EActorCanvasProjectionMode::ActorBoundingBox)
				{
					IndicatorBox = Component->GetOwner()->GetComponentsBoundingBox();
				}
				else
				{
					IndicatorBox = Component->Bounds.GetBox();
				}

				const FVector ProjectBoxPoint = IndicatorBox.GetCenter() + (IndicatorBox.GetSize() * (IndicatorDescriptor.GetBoundingBoxAnchor() - FVector(0.5)));

				FVector2D OutScreenSpacePosition;
				if (ULocalPlayer::GetPixelPoint(InProjectionData, ProjectBoxPoint, OutScreenSpacePosition, &ScreenSize))
				{
					OutScreenSpacePosition += IndicatorDescriptor.GetScreenSpaceOffset();

					OutScreenPositionWithDepth = FVector(OutScreenSpacePosition.X, OutScreenSpacePosition.Y, FVector::Dist(InProjectionData.ViewOrigin, ProjectBoxPoint));
					return true;
				}

				return false;
			}
		}
	}

	return false;
}

void UIndicatorDescriptor::SetIndicatorManagerComponent(UECRIndicatorManagerComponent* InManager)
{
	// Make sure nobody has set this.
	if (ensure(ManagerPtr.IsExplicitlyNull()))
	{
		ManagerPtr = InManager;
	}
}

void UIndicatorDescriptor::UnregisterIndicator()
{
	if (UECRIndicatorManagerComponent* Manager = ManagerPtr.Get())
	{
		Manager->RemoveIndicator(this);
	}
}