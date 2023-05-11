#include "AnimNotifyState_ECRAbilityQueueListen.h"

#include "Gameplay/Character/ECRPawnControlComponent.h"

void UAnimNotifyState_ECRAbilityQueueListen::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AActor* Actor = MeshComp->GetOwner();
	if (!Actor->HasLocalNetOwner())
	{
		return;
	}

	if (UECRPawnControlComponent* PCC = Actor->FindComponentByClass<UECRPawnControlComponent>())
	{
		PCC->bListenForAbilityQueue = false;
		PCC->AbilityQueueDeltaTime = 0;
	}
}

void UAnimNotifyState_ECRAbilityQueueListen::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                         float TotalDuration,
                                                         const FAnimNotifyEventReference& EventReference)
{
	AActor* Actor = MeshComp->GetOwner();
	if (!Actor->HasLocalNetOwner())
	{
		return;
	}

	if (UECRPawnControlComponent* PCC = Actor->FindComponentByClass<UECRPawnControlComponent>())
	{
		PCC->bListenForAbilityQueue = true;
		PCC->AbilityQueueDeltaTime = AbilityQueueDeltaTime;
	}
}
