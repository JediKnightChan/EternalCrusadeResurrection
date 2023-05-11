// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "FMODBlueprintStatics.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ECRAbilityQueueListen.generated.h"


/**
 * UAnimNotifyState_ECRAbilityQueueListen
 *
 * Set bListenForAbilityQueue to true on PawnControlComponent while active
 */
UCLASS()
class ECR_API UAnimNotifyState_ECRAbilityQueueListen : public UAnimNotifyState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	double AbilityQueueDeltaTime = 0;

public:
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	                         const FAnimNotifyEventReference& EventReference) override;
};
