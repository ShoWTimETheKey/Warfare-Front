# Building the published code

This checkout contains original game code and reviewed project/build metadata.
It omits Unreal Engine, proprietary plugins and the complete game content.
GitHub's Source code archive is therefore not a playable game or a complete
reconstruction of the V86 release. Players should use
[Releases](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest).

The exact included files and publication edits are recorded in
[source-manifest.json](../source-manifest.json). This is a development snapshot,
not the frozen V86 build input. See [LICENSING.md](../LICENSING.md) before copying
engine dependencies or redistributing a build.

Tripo-derived output geometry and contact tables are omitted. The retained code
interfaces use original, geometry-free fallbacks with `SourceDataAvailable=false`;
their optional visual loaders are disabled. Compiling those interfaces does not
restore the omitted models, contact data or original visual result, and no Tripo
output has been converted to MIT-licensed geometry.

## Native backend tests without Unreal Engine

The repository also provides a CMake test target for the included standard-C++
backend components. With CMake and a supported C++ compiler installed, run from
the repository root:

```text
cmake -S . -B out/native
cmake --build out/native --config Release
ctest --test-dir out/native -C Release --output-on-failure
```

These tests require neither Unreal Engine nor game assets. They check only the
selected backend behaviour; they do not execute the engine, renderer or complete
campaign. On 21 October 2026, both Release tests passed locally: 1,872 native
order/rule checks and 224 triangle-traversal checks, using MSVC 19.51 and Windows
SDK 26100. Their result is separate from the Unreal build result below.

## Windows dependencies

1. Obtain **Unreal Engine 5.8.2** from Epic. A separately accepted Epic licence is
   required; this repository supplies no engine source or tools.
2. Install **Visual Studio 2026** with the C++ game-development tools and a
   Windows SDK. The included `Build/VS2026.vsconfig` records the selected
   components. Follow the engine's supported-toolchain requirements.
3. Obtain NVIDIA's **DLSS Unreal Engine plugin 8.7.2 for UE 5.8**, including its
   matching Streamline integration. The reviewed local dependency versions are
   **NGX 310.6.0** and **Streamline 2.11.1**. These packages and their vendor
   licences are not included here. Place the official plugins in their normal
   project `Plugins/` layout, or in the appropriate engine plugin installation.
   Preserve their accompanying licences and notices; do not commit them here.
4. Python is needed only for the repository documentation and source-audit
   checks. Those tools use the Python standard library; Python 3.12 is used by
   the documentation CI. It is not needed to compile C++ with UnrealBuildTool.

The game module requires `DLSSBlueprint`, `StreamlineBlueprint`,
`StreamlineDLSSGBlueprint` and `StreamlineReflexBlueprint` in addition to Epic's
standard modules. Turning off their descriptors without changing those module
dependencies is not a supported dependency-free build.

- [Get Unreal Engine](https://www.unrealengine.com/download)
- [Epic's Visual Studio setup guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine)
- [NVIDIA DLSS Unreal Engine downloads](https://developer.nvidia.com/rtx/dlss/get-started)

## Compile the editor target

From a PowerShell session in the repository root, with the dependencies installed:

```powershell
$ueRoot = 'C:\Program Files\Epic Games\UE_5.8' # Adjust to your engine installation.
$projectFile = (Resolve-Path -LiteralPath '.\WarfareFront.uproject').Path
& "$ueRoot\Engine\Build\BatchFiles\Build.bat" WarfareFrontEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE
```

Build products and generated code belong in ignored `Binaries/`, `Intermediate/`
and `Saved/` directories. Never copy an engine installation, SDK, certificate,
player profile or local build cache into a source commit.

## Validation limits

The exported code was compiled in an independent asset-free validation project
on 21 October 2026: `WarfareFrontEditor Win64 Development`, UE 5.8.2
CL 56702186, MSVC 14.50.35739 and Windows SDK 10.0.26100.0. The editor target
compiled and linked successfully, with existing numeric-conversion and
UDT-calling-convention warnings. NVIDIA dependencies were copied privately
outside the published repository and selected with `AdditionalPluginDirectories`
in the validation descriptor. That machine-specific path is not published.

The source manifest and source audit bind this reviewed snapshot. The build
result is not borrowed from an earlier private project or V86 installer.
The machine-independent validation summary, manifest digest and explicit
non-runtime scope are recorded in [SOURCE-VALIDATION.json](SOURCE-VALIDATION.json).

Even a successful C++ editor-target build cannot demonstrate a complete Cook,
Shipping game or visual result: the default map and many runtime soft references
require assets omitted from this repository. It is expected that an asset-free
checkout cannot start the complete game. Recreating or lawfully obtaining the
missing content is a separate task; the binary package does not grant permission
to extract proprietary components and republish them as source assets.

The previously published V86 focused runtime checks remain evidence for that
binary only. They do not establish source reproducibility, a full natural
campaign playthrough or a hardware performance minimum for a new build.

## Documentation checks

```text
python -m unittest discover -s tests -v
python tools/render_release_docs.py --check
```

The tests require actual manifest-listed source files, not only a statement that
the project is open source. They also retain the frozen V86 release metadata and
historical notes. The documentation workflow does not install Unreal Engine or
the proprietary plugins and is not an Unreal build job.

After reviewing a source edit, keep its text in LF format, update the manifest
hashes with `python tools/update_source_manifest.py`, and run
`python tools/audit_source_export.py`. New source paths require a reviewed
manifest entry with provenance and licence; the update tool does not approve
new files. Re-run the appropriate native or Unreal build for code changes.
