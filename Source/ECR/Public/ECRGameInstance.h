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

public:
	virtual void Init() override;

	virtual void Shutdown() override;
};
