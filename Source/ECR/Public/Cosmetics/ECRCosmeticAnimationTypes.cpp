// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRCosmeticAnimationTypes.h"

TSubclassOf<UAnimInstance> FECRAnimLayerSelectionSet::SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FECRAnimLayerSelectionEntry& Rule : LayerRules)
	{
		if ((Rule.Layer != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Layer;
		}
	}

	return DefaultLayer;
}


TSoftObjectPtr<UAnimMontage> FECRAnimMontageSelectionSet::SelectBestMontage(
	const FGameplayTagContainer& CosmeticTags) const
{
	for (const auto& [Montage, RequiredTags] : MontageRules)
	{
		if ((!Montage.IsNull()) && CosmeticTags.HasAll(RequiredTags))
		{
			return Montage;
		}
	}

	return DefaultMontage;
}

TArray<TSoftObjectPtr<UAnimMontage>> FECRAnimMontageSelectionSet::GetAllMontages()
{
	TArray<TSoftObjectPtr<UAnimMontage>> Montages;
	for (auto [Montage, RequiredTags] : MontageRules)
	{
		Montages.AddUnique(Montage);
	}
	Montages.AddUnique(DefaultMontage);
	return Montages;
}

TSubclassOf<AActor> FECRActorSelectionSet::SelectBestActor(const FGameplayTagContainer& CosmeticTags) const
{
	for (const auto& [ActorClass, RequiredTags] : ActorRules)
	{
		if ((ActorClass != nullptr) && CosmeticTags.HasAll(RequiredTags))
		{
			return ActorClass;
		}
	}

	return DefaultActorClass;
}

USkeletalMesh* FECRMeshSelectionSet::SelectBestMesh(const FGameplayTagContainer& CosmeticTags) const
{
	for (const auto& [Mesh, RequiredTags] : MeshRules)
	{
		if ((Mesh != nullptr) && CosmeticTags.HasAll(RequiredTags))
		{
			return Mesh;
		}
	}

	return DefaultMesh;
}

TSubclassOf<UAnimInstance> FECRAnimInstanceSelectionSet::SelectBestAnimInstance(const FGameplayTagContainer& CosmeticTags) const
{
	for (const auto& [AnimInstance, RequiredTags] : AnimInstanceRules)
	{
		if ((AnimInstance != nullptr) && CosmeticTags.HasAll(RequiredTags))
		{
			return AnimInstance;
		}
	}

	return DefaultAnimInstance;
}
