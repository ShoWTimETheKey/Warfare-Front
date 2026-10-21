using UnrealBuildTool;
public class WarfareFrontStartup : ModuleRules
{
    public WarfareFrontStartup(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new [] { "Core", "WarfareFront" });
    }
}
