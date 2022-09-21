using UnrealBuildTool;

public class ECRCommon : ModuleRules
{
	public ECRCommon(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
			}
		);

		PrivateIncludePaths.AddRange(new string[]
			{ System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private" });
	}
}