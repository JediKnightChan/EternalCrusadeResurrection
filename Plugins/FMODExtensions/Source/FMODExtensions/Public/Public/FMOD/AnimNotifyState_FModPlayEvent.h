// Copyleft: All rights reversed

#pragma once

#include "CoreMinimal.h"
#include "FMODBlueprintStatics.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_FModPlayEvent.generated.h"


/**
 * UAnimNotifyState_FModPlayEvent
 *
 * Play FMod event and automatically stop it when AnimNotifyState ends
 */
UCLASS()
class FMODEXTENSIONS_API UAnimNotifyState_FModPlayEvent : public UAnimNotifyState
{
	GENERATED_BODY()

	/** Follow the skeletal mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	bool bFollow;

	/** Event to play */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	UFMODEvent* Event;

	/** Name of the socket or bone to attach sound to */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FName AttachName;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TMap<USkeletalMeshComponent*, FFMODEventInstance> MeshToEventInstanceMap;

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TMap<USkeletalMeshComponent*, UFMODAudioComponent*> MeshToComponentMap;

public:
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
	                         const FAnimNotifyEventReference& EventReference) override;
};
