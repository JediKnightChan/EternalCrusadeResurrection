// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "ECRWeaponUserInterface.generated.h"

class UECRWeaponInstance;

UCLASS()
class UECRWeaponUserInterface : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UECRWeaponUserInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnWeaponChanged(UECRRangedWeaponInstance* OldWeapon, UECRRangedWeaponInstance* NewWeapon);

private:
	void RebuildWidgetFromWeapon();

private:
	UPROPERTY(Transient)
	TObjectPtr<UECRRangedWeaponInstance> CurrentInstance;
};
