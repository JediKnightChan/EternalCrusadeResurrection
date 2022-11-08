// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ECRMatchPlayerState.generated.h"

/**
 * 
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API AECRMatchPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	/** Display name of the player */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FString DisplayName;

	/** Describes replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
