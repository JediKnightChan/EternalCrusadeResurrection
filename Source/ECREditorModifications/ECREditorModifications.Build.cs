using UnrealBuildTool;

public class ECREditorModifications : ModuleRules
{
	public ECREditorModifications(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "Engine",
                "GameplayTags",
                "UnrealEd",
                "PythonScriptPlugin"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Blutility",
				"ECRCommon"
			}
		);

		PrivateIncludePaths.AddRange(new string[]
		{
			System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private"
		});
	}
}