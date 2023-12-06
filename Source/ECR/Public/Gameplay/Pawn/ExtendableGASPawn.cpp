#include "Gameplay/Pawn/ExtendableGASPawn.h"

#include "Gameplay/Character/ECRPawnData.h"
#include "Gameplay/GAS/ECRAbilitySet.h"
#include "Gameplay/GAS/ECRAbilitySystemComponent.h"
#include "Gameplay/GAS/Attributes/ECRCharacterHealthSet.h"
#include "Gameplay/GAS/Attributes/ECRCombatSet.h"
#include "Gameplay/GAS/Components/ECRHealthComponent.h"
#include "Net/UnrealNetwork.h"

AExtendableGASPawn::AExtendableGASPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UECRAbilitySystemComponent>(
		this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CreateDefaultSubobject<UECRCharacterHealthSet>(TEXT("HealthSet"));
	CreateDefaultSubobject<UECRCombatSet>(TEXT("CombatSet"));

	PawnExtComponent->OnAbilitySystemInitialized_RegisterAndCall(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtComponent->OnAbilitySystemUninitialized_Register(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	HealthComponent = CreateDefaultSubobject<UECRHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::K2_OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::K2_OnDeathFinished);
}

UECRAbilitySystemComponent* AExtendableGASPawn::GetECRAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAbilitySystemComponent* AExtendableGASPawn::GetAbilitySystemComponent() const
{
	return GetECRAbilitySystemComponent();
}

void AExtendableGASPawn::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		ECRASC->GetOwnedGameplayTags(TagContainer);
	}
}

bool AExtendableGASPawn::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasMatchingGameplayTag(TagToCheck);
	}

	return false;
}

bool AExtendableGASPawn::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAllMatchingGameplayTags(TagContainer);
	}

	return false;
}

bool AExtendableGASPawn::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	if (const UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent())
	{
		return ECRASC->HasAnyMatchingGameplayTags(TagContainer);
	}

	return false;
}

void AExtendableGASPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
}

void AExtendableGASPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		PawnExtComponent->SetPawnData(PawnData);
	}
}

void AExtendableGASPawn::GrantAbilitySets(const TArray<UECRAbilitySet*> AbilitySets) const
{
	for (const UECRAbilitySet* AbilitySet : AbilitySets)
	{
		if (AbilitySystemComponent && AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}
}

void AExtendableGASPawn::InitPawnDataAndAbilities()
{
	ensureMsgf(PawnData, TEXT("ECRCharacter [%s] pawn data is not specified"), *(GetNameSafe(this)));

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);

	// Granting this character ability sets
	GrantAbilitySets(PawnData->AbilitySets);

	ForceNetUpdate();
}

void AExtendableGASPawn::OnAbilitySystemInitialized()
{
	UECRAbilitySystemComponent* ECRASC = GetECRAbilitySystemComponent();
	check(ECRASC);

	if (GetWorld()->GetNetMode() < NM_Client)
	{
		InitPawnDataAndAbilities();
	}

	HealthComponent->InitializeWithAbilitySystem(ECRASC);
}

void AExtendableGASPawn::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AExtendableGASPawn::OnRep_PawnData()
{
}
