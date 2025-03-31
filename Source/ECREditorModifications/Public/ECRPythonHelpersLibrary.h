#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "ECRPythonHelpersLibrary.generated.h"

UCLASS()
class ECREDITORMODIFICATIONS_API UECRPythonHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameplayTags", meta = (ScriptMethod))
	static FGameplayTag ConvertStringToGameplayTag(UObject* AnyObject, FString TagName);

	UFUNCTION(BlueprintCallable, Category = "Data Tables", meta = (ScriptMethod))
	static void ExportDataTableAsCsv(UDataTable* DataTable, FString OutputPath);
	
	UFUNCTION(BlueprintCallable, Category="Blueprints", meta=(ScriptMethod))
    static UClass* GetParentClass(UBlueprint* Blueprint);
};
