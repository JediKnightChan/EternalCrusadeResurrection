// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CommonGame : ModuleRules
{
	public CommonGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;				
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"CommonInput",
				"CommonUI",
				"CommonUser",
				"GameplayTags",
				"ModularGameplayActors",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
