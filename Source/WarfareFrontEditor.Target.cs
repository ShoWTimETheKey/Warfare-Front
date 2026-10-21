using UnrealBuildTool;
using System.Collections.Generic;

public class WarfareFrontEditorTarget : TargetRules
{
    public WarfareFrontEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("WarfareFront");
        ExtraModuleNames.Add("WarfareFrontStartup");
    }
}
