# Warfare Front

A standalone First World War trench strategy game for Windows.

**[Download the latest complete game](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest)**

Under **Assets**, download `Warfare-Front-v36-Windows-x64.zip`. Extract the **entire ZIP**, then double-click **Play.cmd** or **WarfareFront.exe**. No Unreal Engine, Epic Games Launcher, development tools or game account are required. The game runs offline.

Do **not** download GitHub's automatic `Source code` archives to play. This repository distributes finished game packages, not the development project.

## Language

The interface defaults to **English**. Choose **Chinese** in Settings to switch immediately. Battlefield voices remain English and German. Release notes and diagnostic logs are in English.

## Requirements

- Windows 10 22H2 or Windows 11, 64-bit.
- A DirectX 12 / Shader Model 6.6 GPU with current vendor drivers. A recent discrete GPU is recommended.
- DLSS, Ray Reconstruction and Frame Generation depend on compatible NVIDIA hardware and drivers. Unsupported features fall back to native temporal rendering.
- HDR is optional and requires a compatible display with Windows HDR enabled.
- First launch adapts to desktop resolution and refresh rate; larger desktops use a supported mode within a 4K pixel budget. Display, graphics, frame cap, HDR, audio and language can be changed in Settings.

If normal launch has display problems, close the game and run **Safe Mode.cmd** for a 1080p SDR window with NVIDIA features disabled. This does not overwrite normal display preferences. It cannot make hardware without DX12 / SM6.6 support compatible.

The package includes the required Microsoft x64 C++ runtime. Windows N may require Microsoft's optional Media Feature Pack. This is not a native macOS, Linux or ARM64 release. Physical testing on every GPU, driver and display combination has not been performed.

## Updating and saves

Each release ZIP is a complete game. Extract updates into a new folder; an older version is not needed. Campaign progress remains in `%LOCALAPPDATA%\Warfare1917\Progression-v13.json`; settings and logs are under `%LOCALAPPDATA%\WarfareFront\Saved\`.

Use **Check for updates** in the game settings to compare your version with the latest public GitHub release and open its download page. This optional check requires internet access; it does not download or install anything automatically. Campaign archives display their difficulty and support confirmed individual or batch deletion.

See the [release notes](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest) for changes and the supplied `SHA256SUMS.txt` to verify your download.

## Credits and licence

Warfare Front is an independent historical game, not an official Armor Games or Warfare 1917 / Warfare 1944 release, and is not endorsed by their rightsholders. The game contains war violence and blood. Archival-style transition images are generated period reconstructions, not authentic historical photographs.

Free personal play and redistribution of the complete, unmodified release with all notices are subject to `GAME-LICENSE.txt`, `THIRD-PARTY-NOTICES.md` and `Licenses/` inside the ZIP. Third-party components retain their respective licences. Unreal Engine and NVIDIA DLSS / Reflex belong to their respective rightsholders.
