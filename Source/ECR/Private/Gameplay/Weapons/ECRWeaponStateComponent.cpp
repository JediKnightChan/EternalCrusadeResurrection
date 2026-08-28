// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRWeaponStateComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/Pawn.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"
#include "NativeGameplayTags.h"
#include "Physics/PhysicalMaterialWithTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Gameplay_Zone, "Gameplay.Zone");

UECRWeaponStateComponent::UECRWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);

	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UECRWeaponStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (APawn* Pawn = GetPawn<APawn>())
	{
		if (UECREquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<
			UECREquipmentManagerComponent>())
		{
			if (UECRRangedWeaponInstance* CurrentWeapon = Cast<UECRRangedWeaponInstance>(
				EquipmentManager->GetFirstInstanceOfType(UECRRangedWeaponInstance::StaticClass())))
			{
				CurrentWeapon->Tick(DeltaTime);
			}
		}
	}
}

void UECRWeaponStateComponent::AddLocalSideHitMarkers(const UObject* SourceObject,
                                                      const TArray<FHitResult>& FoundHits)
{
	if (APlayerController* OwnerPC = GetController<APlayerController>())
	{
		bool bUpdatedInstigatedTime = false;

		for (const FHitResult& Hit : FoundHits)
		{
			FVector2D HitScreenLocation;
			if (!UGameplayStatics::ProjectWorldToScreen(OwnerPC, Hit.Location, /*out*/ HitScreenLocation,
			                                            /*bPlayerViewportRelative=*/ false))
			{
				continue;
			}

			// Only need to do this once
			if (!bUpdatedInstigatedTime)
			{
				ActuallyUpdateDamageInstigatedTime();
				bUpdatedInstigatedTime = true;
			}

			FECRScreenSpaceHitLocation& Entry =
				LastWeaponDamageScreenLocations.AddDefaulted_GetRef();

			Entry.Location = HitScreenLocation;
			Entry.HitSuccess = ShouldShowHitAsSuccess(SourceObject, Hit);

			if (const UPhysicalMaterialWithTags* PhysMatWithTags =
				Cast<const UPhysicalMaterialWithTags>(
					Hit.PhysMaterial.Get()))
			{
				for (const FGameplayTag MaterialTag :
				     PhysMatWithTags->Tags)
				{
					if (MaterialTag.MatchesTag(TAG_Gameplay_Zone))
					{
						Entry.HitZone = MaterialTag;
						break;
					}
				}
			}
		}
	}
}

void UECRWeaponStateComponent::ClientDrawServerHit_Implementation(FVector WorldHitLocation, EHitSuccess HitSuccess,
	FGameplayTag HitZone)
{
	APlayerController* OwnerPC = GetController<APlayerController>();
	if (!OwnerPC)
	{
		return;
	}

	FVector2D ScreenLocation;
	if (!UGameplayStatics::ProjectWorldToScreen(
			OwnerPC,
			WorldHitLocation,
			ScreenLocation,
			/*bPlayerViewportRelative=*/ false))
	{
		return;
	}

	// Update damage timing (groups hit markers correctly)
	ActuallyUpdateDamageInstigatedTime();

	FECRScreenSpaceHitLocation& Entry =
		LastWeaponDamageScreenLocations.AddDefaulted_GetRef();

	Entry.Location = ScreenLocation;
	Entry.HitSuccess = HitSuccess;
	Entry.HitZone = HitZone;
}

void UECRWeaponStateComponent::UpdateDamageInstigatedTime(const FGameplayEffectContextHandle& EffectContext)
{
	if (ShouldUpdateDamageInstigatedTime(EffectContext))
	{
		ActuallyUpdateDamageInstigatedTime();
	}
}

bool UECRWeaponStateComponent::ShouldUpdateDamageInstigatedTime_Implementation(
	const FGameplayEffectContextHandle& EffectContext) const
{
	return EffectContext.GetEffectCauser() != nullptr;
}

void UECRWeaponStateComponent::ActuallyUpdateDamageInstigatedTime()
{
	// If our LastWeaponDamageInstigatedTime was not very recent, clear our LastWeaponDamageScreenLocations array
	UWorld* World = GetWorld();
	if (World->GetTimeSeconds() - LastWeaponDamageInstigatedTime > 0.1)
	{
		LastWeaponDamageScreenLocations.Reset();
	}
	LastWeaponDamageInstigatedTime = World->GetTimeSeconds();
}

double UECRWeaponStateComponent::GetTimeSinceLastHitNotification() const
{
	UWorld* World = GetWorld();
	return World->TimeSince(LastWeaponDamageInstigatedTime);
}

EHitSuccess UECRWeaponStateComponent::ShouldShowHitAsSuccess_Implementation(const UObject* SourceObject, const FHitResult& Hit) const
{
	return None;
}
