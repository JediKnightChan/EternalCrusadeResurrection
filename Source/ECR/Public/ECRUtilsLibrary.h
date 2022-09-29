// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "ECRUtilsLibrary.generated.h"

/**
 * 
 */
UCLASS()
// ReSharper disable once CppUE4CodingStandardNamingViolationWarning
class ECR_API UECRUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** Get GUI Supervisor for World, call like: UECRUtilsLibrary::GetGUISupervisor(GetWorld()) */
	static class AECRGUIPlayerController* GetGUISupervisor(const UWorld* World);
};
