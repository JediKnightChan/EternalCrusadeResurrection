// Copyright Epic Games, Inc. All Rights Reserved.

#include "ECRTaggedActor.h"

AECRTaggedActor::AECRTaggedActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AECRTaggedActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(StaticGameplayTags);
}

#if WITH_EDITOR
bool AECRTaggedActor::CanEditChange(const FProperty* InProperty) const
{
	// Prevent editing of the other tags property
	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AActor, Tags))
	{
		return false;
	}

	return Super::CanEditChange(InProperty);
}
#endif
