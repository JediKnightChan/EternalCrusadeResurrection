// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRControllerComponent_CharacterParts.h"
#include "ECRPawnComponent_CharacterParts.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CheatManager.h"

//////////////////////////////////////////////////////////////////////

UECRControllerComponent_CharacterParts::UECRControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UECRControllerComponent_CharacterParts::BeginPlay()
{
	Super::BeginPlay();

	// Listen for pawn possession changed events
	if (HasAuthority())
	{
		if (AController* OwningController = GetController<AController>())
		{
			OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);

			if (APawn* ControlledPawn = GetPawn<APawn>())
			{
				OnPossessedPawnChanged(nullptr, ControlledPawn);
			}
		}
	}
}

void UECRControllerComponent_CharacterParts::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveAllCharacterParts();
	Super::EndPlay(EndPlayReason);
}

UECRPawnComponent_CharacterParts* UECRControllerComponent_CharacterParts::GetPawnCustomizer() const
{
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		return ControlledPawn->FindComponentByClass<UECRPawnComponent_CharacterParts>();
	}
	return nullptr;
}

void UECRControllerComponent_CharacterParts::AddCharacterPart(const FECRCharacterPart& NewPart)
{
	AddCharacterPartInternal(NewPart, ECharacterPartSource::Natural);
}

void UECRControllerComponent_CharacterParts::AddCharacterPartInternal(const FECRCharacterPart& NewPart, ECharacterPartSource Source)
{
	FECRControllerCharacterPartEntry& NewEntry = CharacterParts.AddDefaulted_GetRef();
	NewEntry.Part = NewPart;
	NewEntry.Source = Source;

	if (UECRPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		if (NewEntry.Source != ECharacterPartSource::NaturalSuppressedViaCheat)
		{
			NewEntry.Handle = PawnCustomizer->AddCharacterPart(NewPart);
		}
	}

}

void UECRControllerComponent_CharacterParts::RemoveCharacterPart(const FECRCharacterPart& PartToRemove)
{
	for (auto EntryIt = CharacterParts.CreateIterator(); EntryIt; ++EntryIt)
	{
		if (FECRCharacterPart::AreEquivalentParts(EntryIt->Part, PartToRemove))
		{
			if (UECRPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
			{
				PawnCustomizer->RemoveCharacterPart(EntryIt->Handle);
			}

			EntryIt.RemoveCurrent();
			break;
		}
	}
}

void UECRControllerComponent_CharacterParts::RemoveAllCharacterParts()
{
	if (UECRPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		for (FECRControllerCharacterPartEntry& Entry : CharacterParts)
		{
			PawnCustomizer->RemoveCharacterPart(Entry.Handle);
		}
	}

	CharacterParts.Reset();
}

void UECRControllerComponent_CharacterParts::SetAdditionalCosmeticTags(const FGameplayTagContainer NewTags)
{
	if (UECRPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizer())
	{
		PawnCustomizer->SetAdditionalCosmeticTags(NewTags);
	}
}

void UECRControllerComponent_CharacterParts::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	// Remove parts from the old pawn
	if (UECRPawnComponent_CharacterParts* OldCustomizer = OldPawn ? OldPawn->FindComponentByClass<UECRPawnComponent_CharacterParts>() : nullptr)
	{
		for (FECRControllerCharacterPartEntry& Entry : CharacterParts)
		{
			OldCustomizer->RemoveCharacterPart(Entry.Handle);
			Entry.Handle.Reset();
		}
	}

	// Apply parts to the new pawn
	if (UECRPawnComponent_CharacterParts* NewCustomizer = NewPawn ? NewPawn->FindComponentByClass<UECRPawnComponent_CharacterParts>() : nullptr)
	{
		for (FECRControllerCharacterPartEntry& Entry : CharacterParts)
		{
			ensure(!Entry.Handle.IsValid());

			if (Entry.Source != ECharacterPartSource::NaturalSuppressedViaCheat)
			{
				Entry.Handle = NewCustomizer->AddCharacterPart(Entry.Part);
			}
		}
	}
}
