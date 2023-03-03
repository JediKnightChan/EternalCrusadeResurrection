// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UGameplayTagsManager;

/**
 * FECRGameplayTags
 *
 *	Singleton containing native gameplay tags.
 */
struct FECRGameplayTags
{
public:
	static const FECRGameplayTags& Get() { return GameplayTags; }

	static void InitializeNativeTags();

	static FGameplayTag FindTagByString(FString TagString, bool bMatchPartialString = false);

public:
	FGameplayTag Ability_ActivateFail_IsDead;
	FGameplayTag Ability_ActivateFail_Cooldown;
	FGameplayTag Ability_ActivateFail_Cost;
	FGameplayTag Ability_ActivateFail_TagsBlocked;
	FGameplayTag Ability_ActivateFail_TagsMissing;
	FGameplayTag Ability_ActivateFail_Networking;
	FGameplayTag Ability_ActivateFail_ActivationGroup;

	FGameplayTag Ability_Behavior_SurvivesDeath;

	FGameplayTag InputTag_Move;
	FGameplayTag InputTag_Look_Mouse;
	FGameplayTag InputTag_Look_Stick;
	FGameplayTag InputTag_Crouch;
	FGameplayTag InputTag_AutoRun;

	FGameplayTag GameplayEvent_Death;
	FGameplayTag GameplayEvent_Landed;
	FGameplayTag GameplayEvent_MovementModeChanged;
	FGameplayTag GameplayEvent_RequestReset;
	FGameplayTag GameplayEvent_Reset;
	FGameplayTag GameplayEvent_Wounded;

	FGameplayTag Cosmetic_Montage;
	FGameplayTag Cosmetic_AnimStyle;
	FGameplayTag Cosmetic_ActorSubclass;

	FGameplayTag Cheat_GodMode;
	FGameplayTag Cheat_UnlimitedHealth;

	FGameplayTag SetByCaller_Damage;
	FGameplayTag SetByCaller_Heal;

	FGameplayTag Status_Crouching;
	FGameplayTag Status_ADS;
	FGameplayTag Status_AutoRunning;
	FGameplayTag Status_Death;
	FGameplayTag Status_Death_Dying;
	FGameplayTag Status_Death_Dead;
	FGameplayTag Status_Wounded;

	FGameplayTag Movement_Mode_Walking;
	FGameplayTag Movement_Mode_NavWalking;
	FGameplayTag Movement_Mode_Falling;
	FGameplayTag Movement_Mode_Swimming;
	FGameplayTag Movement_Mode_Flying;
	FGameplayTag Movement_Mode_Custom;

	TMap<uint8, FGameplayTag> MovementModeTagMap;
	TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

protected:
	void AddAllTags(UGameplayTagsManager& Manager);
	void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment);
	void AddMovementModeTag(FGameplayTag& OutTag, const ANSICHAR* TagName, uint8 MovementMode);
	void AddCustomMovementModeTag(FGameplayTag& OutTag, const ANSICHAR* TagName, uint8 CustomMovementMode);

private:
	static FECRGameplayTags GameplayTags;
};
