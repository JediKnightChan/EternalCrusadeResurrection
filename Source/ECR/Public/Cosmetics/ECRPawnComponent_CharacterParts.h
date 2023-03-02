// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Components/PawnComponent.h"
#include "ECRCharacterPartTypes.h"
#include "GameplayTagContainer.h"
#include "Cosmetics/ECRCosmeticAnimationTypes.h"

#include "ECRPawnComponent_CharacterParts.generated.h"

class USkeletalMeshComponent;
class UChildActorComponent;
class UECRPawnComponent_CharacterParts;
struct FECRCharacterPartList;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FECRSpawnedCharacterPartsChanged, UECRPawnComponent_CharacterParts*,
                                            ComponentWithChangedParts);

//////////////////////////////////////////////////////////////////////

// A single applied character part
USTRUCT()
struct FECRAppliedCharacterPartEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FECRAppliedCharacterPartEntry()
	{
	}

	FString GetDebugString() const;

private:
	friend FECRCharacterPartList;
	friend UECRPawnComponent_CharacterParts;

private:
	// The character part being represented
	UPROPERTY()
	FECRCharacterPart Part;

	// Handle index we returned to the user (server only)
	UPROPERTY(NotReplicated)
	int32 PartHandle = INDEX_NONE;

	// The spawned actor instance (client only)
	UPROPERTY(NotReplicated)
	TObjectPtr<UChildActorComponent> SpawnedComponent = nullptr;
};

//////////////////////////////////////////////////////////////////////

// Replicated list of applied character parts
USTRUCT(BlueprintType)
struct FECRCharacterPartList : public FFastArraySerializer
{
	GENERATED_BODY()

	FECRCharacterPartList()
		: OwnerComponent(nullptr)
	{
	}

	FECRCharacterPartList(UECRPawnComponent_CharacterParts* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FECRAppliedCharacterPartEntry, FECRCharacterPartList>(
			Entries, DeltaParms, *this);
	}

	FECRCharacterPartHandle AddEntry(FECRCharacterPart NewPart);
	void RemoveEntry(FECRCharacterPartHandle Handle);
	void ClearAllEntries(bool bBroadcastChangeDelegate);

	FGameplayTagContainer CollectCombinedTags() const;
	void UpdatePlayerStateOnEntries();

private:
	friend UECRPawnComponent_CharacterParts;

	bool SpawnActorForEntry(FECRAppliedCharacterPartEntry& Entry);
	bool DestroyActorForEntry(FECRAppliedCharacterPartEntry& Entry);

private:
	// Replicated list of equipment entries
	UPROPERTY()
	TArray<FECRAppliedCharacterPartEntry> Entries;

	// The component that contains this list
	UPROPERTY()
	UECRPawnComponent_CharacterParts* OwnerComponent;

	// Upcounter for handles
	int32 PartHandleCounter = 0;
};

template <>
struct TStructOpsTypeTraits<FECRCharacterPartList> : public TStructOpsTypeTraitsBase2<FECRCharacterPartList>
{
	enum { WithNetDeltaSerializer = true };
};

//////////////////////////////////////////////////////////////////////

// A component that handles spawning cosmetic actors attached to the owner pawn on all clients
UCLASS(meta=(BlueprintSpawnableComponent))
class UECRPawnComponent_CharacterParts : public UPawnComponent
{
	GENERATED_BODY()

public:
	UECRPawnComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	// Adds a character part to the actor that owns this customization component, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	FECRCharacterPartHandle AddCharacterPart(const FECRCharacterPart& NewPart);

	// Removes a previously added character part from the actor that owns this customization component, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveCharacterPart(FECRCharacterPartHandle Handle);

	// Removes all added character parts, should be called on the authority only
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Cosmetics)
	void RemoveAllCharacterParts();

	// Gets the list of all spawned character parts from this component
	UFUNCTION(BlueprintCallable, BlueprintPure=false, BlueprintCosmetic, Category=Cosmetics)
	TArray<AActor*> GetCharacterPartActors() const;

	// If the parent actor is derived from ACharacter, returns the Mesh component, otherwise nullptr
	USkeletalMeshComponent* GetParentMeshComponent() const;

	// Returns the scene component to attach the spawned actors to
	// If the parent actor is derived from ACharacter, we'll use the Mesh component, otherwise the root component
	USceneComponent* GetSceneComponentToAttachTo() const;

	// Returns the set of combined gameplay tags from attached character parts, optionally filtered to only tags that start with the specified root
	UFUNCTION(BlueprintCallable, BlueprintPure=false, BlueprintCosmetic, Category=Cosmetics)
	FGameplayTagContainer GetCombinedTags(FGameplayTag RequiredPrefix) const;

	// Among spawned actors that depend on player state, update player state reference
	UFUNCTION(BlueprintCallable)
	void UpdatePlayerStateOnEntries();

	void BroadcastChanged();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetAdditionalCosmeticTags(const FGameplayTagContainer NewTags);

public:
	// Delegate that will be called when the list of spawned character parts has changed
	UPROPERTY(BlueprintAssignable, Category=Cosmetics, BlueprintCallable)
	FECRSpawnedCharacterPartsChanged OnCharacterPartsChanged;

private:
	UFUNCTION()
	void OnRep_AdditionalCosmeticTags();

private:
	// List of character parts
	UPROPERTY(Replicated)
	FECRCharacterPartList CharacterPartList;

	// Rules for how to pick a body style mesh for animation to play on, based on character part cosmetics tags
	UPROPERTY(EditAnywhere, Category=Cosmetics)
	FECRAnimBodyStyleSelectionSet BodyMeshes;

	// Additional cosmetic tags not depending on spawned character parts
	UPROPERTY(ReplicatedUsing=OnRep_AdditionalCosmeticTags)
	FGameplayTagContainer AdditionalCosmeticTags;
};
