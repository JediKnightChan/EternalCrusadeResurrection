// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECR : ModuleRules
{
	public ECR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", 
			"CoreUObject", 
			"Engine",
			"PhysicsCore",
			"HeadMountedDisplay", 
			"ECRCommon", 
			"OnlineSubsystem",
			"OnlineSubsystemEOS", 
			"OnlineSubsystemUtils",
			"ModularGameplay",
			"ModularGameplayActors",
			"SignificanceManager",
			"Niagara"
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] {
			"InputCore",
			"Slate",
			"SlateCore",
			"NetCore",
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks", 
			"GameplayMessageRuntime",
			"EnhancedInput", 
			"CommonUI",
			"CommonInput", 
			"CommonGame",
			"CommonUser",
			"AudioMixer", "Networking"
		});
	}
}