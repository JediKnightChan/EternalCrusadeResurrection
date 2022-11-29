// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ECRPlayerController.generated.h"

/**
 * 
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API AECRPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
};
