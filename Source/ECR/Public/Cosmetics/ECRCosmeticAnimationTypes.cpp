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

USkeletalMesh* FECRAnimBodyStyleSelectionSet::SelectBestBodyStyle(const FGameplayTagContainer& CosmeticTags) const
{
	for (const FECRAnimBodyStyleSelectionEntry& Rule : MeshRules)
	{
		if ((Rule.Mesh != nullptr) && CosmeticTags.HasAll(Rule.RequiredTags))
		{
			return Rule.Mesh;
		}
	}

	return DefaultMesh;
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
