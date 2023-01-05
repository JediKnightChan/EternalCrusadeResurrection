// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "ECRHealthSet.h"
#include "ECRSimpleHealthSet.generated.h"

/**
 * Simple health set for actors like door that can be broken, etc.
 */
UCLASS()
class ECR_API UECRSimpleHealthSet : public UECRHealthSet
{
	GENERATED_BODY()
};
