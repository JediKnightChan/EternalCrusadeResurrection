// Copyleft: All rights reversed

#include "FMOD/AnimNotifyState_FModPlayEvent.h"
#include "FMODBlueprintStatics.h"

void UAnimNotifyState_FModPlayEvent::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (UFMODAudioComponent* AudioComponent = MeshToComponentMap.FindRef(MeshComp))
	{
		AudioComponent->Stop();
		MeshToComponentMap.Remove(MeshComp);
	}
	
	if (MeshToEventInstanceMap.Contains(MeshComp)) {
		UFMODBlueprintStatics::EventInstanceStop(MeshToEventInstanceMap.FindRef(MeshComp));
		MeshToEventInstanceMap.Remove(MeshComp);
	}
}

void UAnimNotifyState_FModPlayEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                 float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (bFollow)
	{
		UFMODAudioComponent* AudioComponent = UFMODBlueprintStatics::PlayEventAttached(Event, MeshComp, AttachName, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, false, true, true);
		MeshToComponentMap.Add(MeshComp, AudioComponent);
	} else
	{
		const FFMODEventInstance EventInstance = UFMODBlueprintStatics::PlayEventAtLocation(GetWorld(), Event, MeshComp->GetComponentTransform(), true);
		MeshToEventInstanceMap.Add(MeshComp, EventInstance);
	}
}
