// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "CustomizationElementaryModule.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(ModularCustomization), meta=(BlueprintSpawnableComponent))
class ECRCOMMON_API UCustomizationElementaryModule : public USkeletalMeshComponent
{
	GENERATED_BODY()

	/* Whether this module wants to be saved */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bWantsSave;

	/* Whether to inherit animations from first SkeletalMeshComponent parent */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bInheritAnimations;

	/* Whether this module should be saved even if it has no children - WORKAROUND OnRegister called with 0 children */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bAllowSaveWithoutChildren;
protected:
	virtual void OnRegister() override;

	/* Inheriting animations from first SkeletalMeshComponent parent if requested */
	void InheritAnimationsIfNeeded();

	/* Save this module into a CustomizationElementaryAsset */
	void SaveToDataAsset() const;

public:
	UCustomizationElementaryModule();
};
