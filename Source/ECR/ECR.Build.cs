// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECR : ModuleRules
{
	public ECR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"ECRCommon", 
			"Engine", 
			"HeadMountedDisplay", 
			"ModularGameplay", 
			"ModularGameplayActors", 
			"Niagara", 
			"OnlineSubsystem", 
			"OnlineSubsystemEOS", 
			"OnlineSubsystemUtils", 
			"PhysicsCore", 
			"SignificanceManager"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule", 
			"AudioMixer", 
			"CommonGame", 
			"CommonInput", 
			"CommonUI",
			"CommonUser", 
			"DeveloperSettings",
			"EnhancedInput", 
			"GameplayAbilities", 
			"GameplayMessageRuntime", 
			"GameplayTags", 
			"GameplayTags",
			"GameplayTasks", 
			"GameplayTasks", 
			"InputCore", 
			"NetCore", 
			"Networking", 
			"Slate", 
			"SlateCore"
		});
	}
}