// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ECRTarget : TargetRules
{
	public ECRTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		// Enable logs for Shipping builds
		if (Configuration == UnrealTargetConfiguration.Shipping)
		{
			BuildEnvironment = TargetBuildEnvironment.Unique;
			bUseLoggingInShipping = true;
		}

		ExtraModuleNames.Add("ECR");
		RegisterModulesCreatedByRider();
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[] { "ECRCommon" });
	}
}