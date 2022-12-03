// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ECR : ModuleRules
{
	public ECR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ECRCommon", "OnlineSubsystem",
			"OnlineSubsystemEOS", "OnlineSubsystemUtils", "Slate"
		});
		PrivateDependencyModuleNames.AddRange(new string[]
			{ "GameplayAbilities", "GameplayTags", "GameplayTasks" });
	}
}