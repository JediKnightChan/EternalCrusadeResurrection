// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"
#include "ECRCharacterMovementComponent.generated.h"

ECR_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);


/**
 * FECRCharacterGroundInfo
 *
 *	Information about the ground under the character.  It only gets updated as needed.
 */
USTRUCT(BlueprintType)
struct FECRCharacterGroundInfo
{
	GENERATED_BODY()

	FECRCharacterGroundInfo()
		: LastUpdateFrame(0),
		  GroundDistance(0.0f)
	{
	}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};


/**
 * UECRCharacterMovementComponent
 *
 *	The base character movement component class used by this project.
 */
UCLASS(Config = Game)
class ECR_API UECRCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UECRCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	virtual bool CanAttemptJump() const override;

	// Returns the current ground info.  Calling this will update the ground info if it's out of date.
	UFUNCTION(BlueprintCallable, Category = "ECR|CharacterMovement")
	const FECRCharacterGroundInfo& GetGroundInfo();

	//~UMovementComponent interface
	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;
	virtual bool ShouldUsePackedMovementRPCs() const override;
	//~End of UMovementComponent interface

protected:
	virtual void ApplyImpactPhysicsForces(const FHitResult& Impact, const FVector& ImpactAcceleration,
	                                      const FVector& ImpactVelocity) override;


	virtual void InitializeComponent() override;

protected:
	// Cached ground info for the character.  Do not access this directly!  It's only updated when accessed via GetGroundInfo().
	FECRCharacterGroundInfo CachedGroundInfo;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bDontApplyImpactOnVehicles;
};
