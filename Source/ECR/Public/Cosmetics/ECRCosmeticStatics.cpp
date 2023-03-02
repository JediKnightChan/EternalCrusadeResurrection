#include "ECRCosmeticStatics.h"

#include "ECRPawnComponent_CharacterParts.h"

UECRPawnComponent_CharacterParts* UECRCosmeticStatics::GetPawnCustomizationComponentFromActor(AActor* TargetActor)
{
	if (TargetActor)
	{
		UActorComponent* ActorCustomizationComponent = TargetActor->GetComponentByClass(
			UECRPawnComponent_CharacterParts::StaticClass());
		if (UECRPawnComponent_CharacterParts* CustomizationComponent = Cast<UECRPawnComponent_CharacterParts>(
			ActorCustomizationComponent))
		{
			return CustomizationComponent;
		}
	}
	return nullptr;
}

void UECRCosmeticStatics::AddMontageToLoadQueueIfNeeded(const TSoftObjectPtr<UAnimMontage>& Montage,
                                                        TArray<FSoftObjectPath>& MontagesToLoad)
{
	if (!Montage.IsNull() && !Montage.IsValid())
	{
		MontagesToLoad.Add(Montage.ToSoftObjectPath());
	};
}
