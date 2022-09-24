// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ECRGameInstance.generated.h"

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
public:
	UECRGameInstance();

	virtual void Init() override;
	/** Login user */
	void Login();

	/** Clear OnLoginComplete delegates when OnLoginComplete fires */
	void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	
	/** Create match, by player (P2P) or dedicated server */
	void CreateMatch(FName MatchName, const int32 NumPublicConnections, const bool bIsDedicated);

	/** Clear OnCreateMatchComplete delegates when OnCreateMatchComplete fires */
	void OnCreateMatchComplete(FName SessionName, bool bWasSuccessful);
};
