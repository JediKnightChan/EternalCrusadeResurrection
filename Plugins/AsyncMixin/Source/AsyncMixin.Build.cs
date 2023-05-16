// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AsyncMixin : ModuleRules
{
	public AsyncMixin(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

        PublicIncludePathModuleNames.AddRange(
            new string[] {
            }
        );
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
