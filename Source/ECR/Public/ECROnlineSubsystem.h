// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ECROnlineSubsystem.generated.h"

/**
 * GameInstance Subsystem handling online functionality (logging in, match creation and joining, ...)
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API UECROnlineSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	/** Online subsystem */
	class IOnlineSubsystem* OnlineSubsystem;

	/** Whether user is logged in */
	bool bIsLoggedIn;

	/** Session search object */
	TSharedPtr<class FOnlineSessionSearch> SessionSearchSettings;

	/** Name assigned to player that will be shown in matches */
	FString UserDisplayName;
	
protected:
	/** Login via selected login type */
	void Login(FString PlayerName, FString LoginType);

	/** Clear OnLoginComplete delegates and show main menu if success when OnLoginComplete fires */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

	/** Clear OnCreateMatchComplete delegates when OnCreateMatchComplete fires */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);

	/** Clear OnFindSessionsComplete delegates when OnFindSessionsComplete fires */
	void OnFindSessionsComplete(bool bWasSuccessful);

public:
	UECROnlineSubsystem();
	
	/** Login user via Epic Account */
	UFUNCTION(BlueprintCallable)
	void LoginViaEpic(FString PlayerName);

	/** Login user via Device ID */
	UFUNCTION(BlueprintCallable)
	void LoginViaDevice(FString PlayerName);

	/** Create match, by player (P2P) or dedicated server */
	UFUNCTION(BlueprintCallable)
	void CreateMatch(const FName SessionName, const int32 NumPublicConnections, const FString MatchType,
	                 const FString MatchMode, const FString MapName);

	/** Create match, by player (P2P) or dedicated server */
	UFUNCTION(BlueprintCallable)
	void FindMatches(const FString MatchType = "", const FString MatchMode = "", const FString MapName = "");
};
