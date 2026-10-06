#include "AnimNotifyState_ECRAbilityQueueListen.h"

#include "Gameplay/Character/ECRPawnControlComponent.h"
#include "Settings/ECRSettingsLocal.h"

void UAnimNotifyState_ECRAbilityQueueListen::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* Actor = MeshComp->GetOwner();
	if (!Actor || !Actor->HasLocalNetOwner())
	{
		return;
	}

	if (UECRSettingsLocal* Settings = UECRSettingsLocal::Get())
	{
		if (!Settings->bEnableAbilityQueue && bRespectsDisableSetting)
		{
			return;
		}
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
	if (!MeshComp)
	{
		return;
	}

	AActor* Actor = MeshComp->GetOwner();
	if (!Actor || !Actor->HasLocalNetOwner())
	{
		return;
	}

	if (UECRSettingsLocal* Settings = UECRSettingsLocal::Get())
	{
		if (!Settings->bEnableAbilityQueue && bRespectsDisableSetting)
		{
			return;
		}
	}

	if (UECRPawnControlComponent* PCC = Actor->FindComponentByClass<UECRPawnControlComponent>())
	{
		PCC->bListenForAbilityQueue = true;
		PCC->AbilityQueueDeltaTime = AbilityQueueDeltaTime;
	}
}
