using UnrealBuildTool;

public class WarfareFront : ModuleRules
{
    public WarfareFront(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Accepted builds compiled every module file separately through UBT's
        // git-driven adaptive mode. Unity blobs merge legacy anonymous
        // namespaces (duplicate helpers/constants) and fail; keep the same
        // translation units whether or not git is on PATH.
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "MeshDescription", "StaticMeshDescription", "Json" });
        PrivateDependencyModuleNames.AddRange(new[] { "AnimationCore", "HTTP", "NavigationSystem" });
        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore", "ApplicationCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "RenderCore", "RHI", "PhysicsCore", "AudioMixer", "SignalProcessing" });
        PrivateDependencyModuleNames.AddRange(new[] { "Renderer", "DLSSBlueprint", "StreamlineBlueprint", "StreamlineDLSSGBlueprint", "StreamlineReflexBlueprint" });
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicSystemLibraries.AddRange(new[] { "dxgi.lib", "dwmapi.lib", "windowscodecs.lib", "ole32.lib", "msimg32.lib" });
            RuntimeDependencies.Add("$(ProjectDir)/Content/Startup/V73/...", StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(ProjectDir)/Content/UI/Cursors/V75/...", StagedFileType.NonUFS);
            RuntimeDependencies.Add("$(ProjectDir)/Content/UI/Cursors/V81/...", StagedFileType.NonUFS);
        }
        if (Target.bBuildEditor) PrivateDependencyModuleNames.AddRange(new[] { "AssetRegistry" });
    }
}
