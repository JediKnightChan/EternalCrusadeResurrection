// Copyleft: All rights reversed

#pragma once

#include "Customization/CustomizationElementaryAsset.h"
#include "CustomizationAttachmentAsset.generated.h"


/**
 * 
 */
UCLASS()
class ECRCOMMON_API UCustomizationAttachmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FName ModuleName;

	/** Array of static attachments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<FCustomizationElementarySubmoduleStatic> StaticAttachments;

	/** Array of skeletal attachments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<FCustomizationElementarySubmoduleSkeletal> SkeletalAttachments;

	/** Array of particle attachments */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TArray<FCustomizationElementarySubmoduleParticle> ParticleAttachments;
};
