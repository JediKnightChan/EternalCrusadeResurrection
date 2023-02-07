// Copyright Epic Games, Inc. All Rights Reserved.

#include "GUI/Weapons/ECRWeaponUserInterface.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Weapons/ECRWeaponInstance.h"
#include "Gameplay/Equipment/ECREquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Weapons/ECRRangedWeaponInstance.h"

UECRWeaponUserInterface::UECRWeaponUserInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UECRWeaponUserInterface::NativeConstruct()
{
	Super::NativeConstruct();
}

void UECRWeaponUserInterface::NativeDestruct()
{
	Super::NativeDestruct();
}

void UECRWeaponUserInterface::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UECREquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UECREquipmentManagerComponent>())
		{
			if (UECRRangedWeaponInstance* NewInstance = EquipmentManager->GetFirstInstanceOfType<UECRRangedWeaponInstance>())
			{
				if (NewInstance != CurrentInstance && NewInstance->GetInstigator() != nullptr)
				{
					UECRRangedWeaponInstance* OldWeapon = CurrentInstance;
					CurrentInstance = NewInstance;
					RebuildWidgetFromWeapon();
					OnWeaponChanged(OldWeapon, CurrentInstance);
				}
			}
		}
	}
}

void UECRWeaponUserInterface::RebuildWidgetFromWeapon()
{
	
}
