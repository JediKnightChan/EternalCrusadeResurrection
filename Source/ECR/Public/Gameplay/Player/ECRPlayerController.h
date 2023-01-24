// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CommonPlayerController.h"
#include "Gameplay/Camera/ECRCameraAssistInterface.h"
#include "ECRPlayerController.generated.h"

class AECRPlayerState;
class UECRAbilitySystemComponent;
class APawn;

/**
 * AECRPlayerController
 *
 *	The base player controller class used by this project.
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class AECRPlayerController : public ACommonPlayerController, public IECRCameraAssistInterface
{
	GENERATED_BODY()

public:

	AECRPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "ECR|PlayerController")
	AECRPlayerState* GetECRPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "ECR|PlayerController")
	UECRAbilitySystemComponent* GetECRAbilitySystemComponent() const;	

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface
	
	//~AController interface
	virtual void OnUnPossess() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void PlayerTick(float DeltaTime) override;
	//~End of APlayerController interface

	//~ACommonPlayerController interface
	virtual void OnPossess(APawn* InPawn) override;
	//~End of ACommonPlayerController interface

	UFUNCTION(BlueprintCallable, Category = "ECR|Character")
	void SetIsAutoRunning(const bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "ECR|Character")
	bool GetIsAutoRunning() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ECR|Network")
	int32 GetInPacketLoss() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="ECR|Network")
	int32 GetOutPacketLoss() const;
protected:
	//~APlayerController interface
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface
	
	void OnStartAutoRun();
	void OnEndAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnStartAutoRun"))
	void K2_OnStartAutoRun();

	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName="OnEndAutoRun"))
	void K2_OnEndAutoRun();
};


// A player controller used for replay capture and playback
UCLASS()
class AECRReplayPlayerController : public AECRPlayerController
{
	GENERATED_BODY()

	virtual void SetPlayer(UPlayer* InPlayer) override;
};
