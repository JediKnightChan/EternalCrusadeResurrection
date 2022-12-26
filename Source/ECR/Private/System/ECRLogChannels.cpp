// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ECRLogChannels.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogECR);
DEFINE_LOG_CATEGORY(LogECRExperience);
DEFINE_LOG_CATEGORY(LogECRAbilitySystem);
DEFINE_LOG_CATEGORY(LogECRTeams);

FString GetClientServerContextString(UObject* ContextObject)
{
	ENetRole Role = ROLE_None;

	if (const AActor* Actor = Cast<AActor>(ContextObject))
	{
		Role = Actor->GetLocalRole();
	}
	else if (const UActorComponent* Component = Cast<UActorComponent>(ContextObject))
	{
		Role = Component->GetOwnerRole();
	}

	if (Role != ROLE_None)
	{
		return (Role == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
	}
	else
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			extern ENGINE_API FString GPlayInEditorContextString;
			return GPlayInEditorContextString;
		}
#endif
	}

	return TEXT("[]");
}
