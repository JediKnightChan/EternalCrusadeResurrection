// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ECRServerTarget : TargetRules
{
    public ECRServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("ECR");
        RegisterModulesCreatedByRider();
    }
    
    private void RegisterModulesCreatedByRider()
    {
        ExtraModuleNames.AddRange(new string[] { "ECRCommon"});
    }
}