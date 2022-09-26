// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ECRGameInstance.generated.h"


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
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API UECRGameInstance : public UGameInstance
{
	GENERATED_BODY()

	/** Online subsystem */
	class IOnlineSubsystem* OnlineSubsystem;

protected:
	/** Whether user is logged in */
	bool bIsLoggedIn;

	/** Login via selected login type */
	void Login(FString LoginType);

	/** Clear OnLoginComplete delegates when OnLoginComplete fires */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** Clear OnCreateMatchComplete delegates when OnCreateMatchComplete fires */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);
public:
	UECRGameInstance();

	/** Get Online Subsystem on Init */
	virtual void Init() override;

	/** If active session, destroy it */
	virtual void Shutdown() override;

	/** Login user via Epic Account */
	UFUNCTION(BlueprintCallable)
	void LoginViaEpic();

	/** Login user via Device ID */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevice();

	/** Create match, by player (P2P) or dedicated server */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const int32 NumPublicConnections, const bool bIsDedicated);

	// Interface actions - blueprint implementable

	/** Show Main menu - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowMainMenu(bool bResetState);

	/** Show Loading screen - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowLoadingScreen(ELoadingScreenType LoadingScreen);

	/** Show Error message - need to override in blueprints */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ShowErrorMessage(const FText& ErrorMessage);
};
