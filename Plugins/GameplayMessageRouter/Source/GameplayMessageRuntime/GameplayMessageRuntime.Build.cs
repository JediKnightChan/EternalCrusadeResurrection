// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameplayMessageRuntime : ModuleRules
{
	public GameplayMessageRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;				

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Engine",
				"GameplayTags"
			});
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
			});
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			});
	}
}
