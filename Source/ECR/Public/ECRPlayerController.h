// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ECRPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ECR_API AECRPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
};
