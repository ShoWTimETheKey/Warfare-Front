using UnrealBuildTool;
using System.Collections.Generic;

public class WarfareFrontTarget : TargetRules
{
    public WarfareFrontTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarfareFront");
        ExtraModuleNames.Add("WarfareFrontStartup");
    }
}
