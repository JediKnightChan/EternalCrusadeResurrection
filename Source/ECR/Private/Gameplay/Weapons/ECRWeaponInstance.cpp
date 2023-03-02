// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Weapons/ECRWeaponInstance.h"

#include "Cosmetics/ECRCosmeticStatics.h"
#include "Cosmetics/ECRPawnComponent_CharacterParts.h"
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Gameplay/ECRGameplayTags.h"
#include "Gameplay/Character/ECRCharacter.h"
#include "System/ECRLogChannels.h"

UECRWeaponInstance::UECRWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UECRWeaponInstance::LinkAnimLayer() const
{
	if (const UECRPawnComponent_CharacterParts* CosmeticComponent =
		UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(GetPawn()))
	{
		const TSubclassOf<UAnimInstance> AnimClass = PickBestAnimLayer(
			CosmeticComponent->GetCombinedTags(FECRGameplayTags::Get().Cosmetic_AnimStyle));
		const AECRCharacter* Character = Cast<AECRCharacter>(GetPawn());

		if (AnimClass && Character)
		{
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				Mesh->LinkAnimClassLayers(AnimClass);
			}
		}
	}
}

void UECRWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	UWorld* World = GetWorld();
	check(World);
	TimeLastEquipped = World->GetTimeSeconds();

	LoadMontages();

	if (UECRPawnComponent_CharacterParts* CosmeticComp = UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(GetPawn()))
	{
		CosmeticComp->OnCharacterPartsChanged.AddDynamic(this, &ThisClass::OnCharacterPartsChanged);
	}

	if (bVisible)
	{
		LinkAnimLayer();
	}
}

void UECRWeaponInstance::OnUnequipped()
{
	Super::OnUnequipped();

	if (UECRPawnComponent_CharacterParts* CosmeticComp = UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(GetPawn()))
	{
		CosmeticComp->OnCharacterPartsChanged.RemoveDynamic(this, &ThisClass::OnCharacterPartsChanged);
	}

	LinkAnimLayer();
}

void UECRWeaponInstance::UpdateFiringTime()
{
	UWorld* World = GetWorld();
	check(World);
	TimeLastFired = World->GetTimeSeconds();
}

float UECRWeaponInstance::GetTimeSinceLastInteractedWith() const
{
	UWorld* World = GetWorld();
	check(World);
	const double WorldTime = World->GetTimeSeconds();

	double Result = WorldTime - TimeLastEquipped;

	if (TimeLastFired > 0.0)
	{
		const double TimeSinceFired = WorldTime - TimeLastFired;
		Result = FMath::Min(Result, TimeSinceFired);
	}

	return Result;
}

TSubclassOf<UAnimInstance> UECRWeaponInstance::PickBestAnimLayer(const FGameplayTagContainer& CosmeticTags) const
{
	const FECRAnimLayerSelectionSet& SetToQuery = (bEquipped ? EquippedAnimSet : UnequippedAnimSet);
	return SetToQuery.SelectBestLayer(CosmeticTags);
}

UAnimMontage* UECRWeaponInstance::GetKillerExecutionMontage() const
{
	return GetExecutionMontage(KillerExecutionMontageSet, GetPawn());
}

UAnimMontage* UECRWeaponInstance::GetVictimExecutionMontage(AActor* TargetActor) const
{
	return GetExecutionMontage(VictimExecutionMontageSet, TargetActor);
}

UAnimMontage* UECRWeaponInstance::GetExecutionMontage(const FECRAnimMontageSelectionSet& SelectionSet,
                                                      AActor* TargetActor) const
{
	if (const UECRPawnComponent_CharacterParts* CosmeticsComponent =
		UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(TargetActor))
	{
		const FGameplayTagContainer PawnCosmeticTags = CosmeticsComponent->GetCombinedTags(
			FECRGameplayTags::Get().Cosmetic_Montage);
		const TSoftObjectPtr<UAnimMontage> AnimMontage = SelectionSet.SelectBestMontage(PawnCosmeticTags);
		if (!AnimMontage.IsNull() && !AnimMontage.IsValid())
		{
			UE_LOG(LogECR, Warning, TEXT("Had to sync load montage %s"), *(AnimMontage.GetAssetName()));
			UAssetManager::GetStreamableManager().RequestSyncLoad(AnimMontage.ToSoftObjectPath());
		}
		return AnimMontage.Get();
	}
	return nullptr;
}

void UECRWeaponInstance::LoadMontages()
{
	TArray<FSoftObjectPath> MontagesToLoad;

	if (const UECRPawnComponent_CharacterParts* CosmeticsComponent =
		UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(GetPawn()))
	{
		const FGameplayTagContainer PawnCosmeticTags = CosmeticsComponent->GetCombinedTags(
			FECRGameplayTags::Get().Cosmetic_Montage);
		const TSoftObjectPtr<UAnimMontage> KillerAnimMontage = KillerExecutionMontageSet.SelectBestMontage(
			PawnCosmeticTags);
		UECRCosmeticStatics::AddMontageToLoadQueueIfNeeded(KillerAnimMontage, MontagesToLoad);

		for (TSoftObjectPtr<UAnimMontage>& VictimAnimMontage : VictimExecutionMontageSet.GetAllMontages())
		{
			UECRCosmeticStatics::AddMontageToLoadQueueIfNeeded(VictimAnimMontage, MontagesToLoad);
		}
	}

	if (MontagesToLoad.Num() > 0)
	{
		UAssetManager::GetStreamableManager().RequestAsyncLoad(MontagesToLoad);
	}
}

void UECRWeaponInstance::OnCharacterPartsChanged(UECRPawnComponent_CharacterParts* ComponentWithChangedParts)
{
	LoadMontages();

	if (bEquipped && bVisible)
	{
		LinkAnimLayer();
	}
}
