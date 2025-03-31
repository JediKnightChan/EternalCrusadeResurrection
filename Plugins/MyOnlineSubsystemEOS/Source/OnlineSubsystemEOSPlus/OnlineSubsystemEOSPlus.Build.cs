// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineSubsystemEOSPlus : ModuleRules
{
	public OnlineSubsystemEOSPlus(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDefinitions.Add("ONLINESUBSYSTEMEOSPLUS_PACKAGE=1");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"OnlineSubsystemUtils"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Sockets",
				"OnlineSubsystem",
				"Json",
				"OnlineSubsystemEOS"
			}
		);

		PrivateDefinitions.Add("CREATE_MIRROR_PLATFORM_SESSION=" + (bCreateMirrorPlatformSession ? "1" : "0"));
	}

	protected virtual bool bCreateMirrorPlatformSession { get { return true; } }
}
