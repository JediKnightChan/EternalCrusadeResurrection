// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Player/ECRPlayerController.h"
#include "Online/ECROnlineSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "ECRGUIPlayerController.generated.h"


/** Type of loading screen */
UENUM(BlueprintType)
enum ELoadingScreenType
{
	LoadingMap,
	LoadingFastData,
};


/**
 * 
 */
UCLASS()
class ECR_API AECRGUIPlayerController : public AECRPlayerController
{
	GENERATED_BODY()

	// Reference to game instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class UECRGameInstance* GameInstance;
public:
	// Handlers for events that refer to GUI - blueprint implementable

	/** Handle error of Login of different types */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HandleLoginFailed(const FString& ErrorMessage);

	/** Handle error of CreateMatch */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HandleCreateMatchFailed();

	/** Handle success of FindMatches */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HandleFindMatchesSuccess(const TArray<FECRMatchResult>& Results);

	/** Handle error of FindMatches */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HandleFindMatchesFailed();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void HandleJoinMatchFailed(bool bSessionIsFull, bool bSessionDoesNotExist);

	// Interface actions - blueprint implementable

	/** Show Main menu - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowLoginMenu();

	/** Show Main menu - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowMainMenu(bool bResetState);

	/** Show Loading screen - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowLoadingScreen(ELoadingScreenType LoadingScreen);
};
