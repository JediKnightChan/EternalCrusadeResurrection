// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "ModularMeshLoaderComponent.generated.h"

/**
 * 
 */
UCLASS()
class ECRCOMMON_API UModularMeshLoaderComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

	bool bInheritAnimations;
public:
	UModularMeshLoaderComponent();
	virtual void BeginPlay() override;
};
