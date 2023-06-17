// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetSystemTargetableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UTargetSystemTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class TARGETSYSTEM_API ITargetSystemTargetableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Target System")
	bool IsTargetable() const;
};
