# Warfare Front

A cinematic First World War trench strategy game for Windows. Lead British or German forces through a branching Western Front campaign, organise your company and coordinate infantry, armour and fire support across shell-torn ground.

**[Download the latest complete game](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest)**

## Download and play

Download **all four files** from the same release:

1. [Warfare-Front-v53-Windows-x64.exe](https://github.com/ShoWTimETheKey/Warfare-Front/releases/download/v53/Warfare-Front-v53-Windows-x64.exe) — offline extractor.
2. [Warfare-Front-v53-Windows-x64.7z.001](https://github.com/ShoWTimETheKey/Warfare-Front/releases/download/v53/Warfare-Front-v53-Windows-x64.7z.001) — game data, part 1.
3. [Warfare-Front-v53-Windows-x64.7z.002](https://github.com/ShoWTimETheKey/Warfare-Front/releases/download/v53/Warfare-Front-v53-Windows-x64.7z.002) — game data, part 2.
4. [Warfare-Front-v53-Windows-x64.7z.003](https://github.com/ShoWTimETheKey/Warfare-Front/releases/download/v53/Warfare-Front-v53-Windows-x64.7z.003) — game data, part 3.

Keep them in **one folder with these exact filenames**. After all downloads finish, double-click the download EXE, choose a writable destination and select **Extract**. Open the extracted game folder and double-click **Play.cmd**. Keep its supplied folders together. Allow free space for both the downloads and the extracted game.

The download EXE only extracts the adjacent numbered parts; it is not the game or a downloader. No installed **7-Zip**, administrator rights, Unreal Engine, Epic Games Launcher, development tools or game account are required. The game runs offline. If extraction reports a missing volume, place that part beside the extractor and retry. Do not mix versions, rename the files or open a single part with Windows ZIP tools.

Do **not** download GitHub's automatic `Source code` archives to play. This repository distributes finished game packages, not the development project.

## The Western Front

- **Choose your assignment.** Four historical forks create a sixteen-operation route through each army's twenty-operation campaign roster. Study the headquarters map, compare commanders, ground and opposition, then commit your company. Completed assignments remain replayable.
- **Build a fighting company.** Combine riflemen, machine guns, command sections and specialists with armour, artillery and air support. Research a fixed or mobile machine-gun organisation, use command and observation bonuses, and choose your attachments before departure.
- **Read the ground.** Advance through cratered mud, chalk, ruins, canals, railway cuttings and snow. Weather, cover, wire, mines, gas and smoke affect the fight. Rain and blasts disturb standing water; casualties can leave dark deposits within connected pools.
- **Carry the campaign forward.** Dispatches offer real reinforcements or research allocations. British and German orders follow the pressures of 1917–1918, with distinct armistice epilogues at the end of each route.

V53 includes all changes since the previous public V49 release, including the strategic map, campaign choices, revised command and machine-gun sections, terrain and water work, and corrected post-victory unlock timing. See the [release notes](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest).

## Language

The interface defaults to **English**. Choose **Chinese** in Settings to switch immediately. Battlefield voices remain English and German. Release notes and diagnostic logs are in English.

## Graphics and requirements

- Windows 10 22H2 or Windows 11, 64-bit, with current GPU drivers.
- Normal launch uses DirectX 12 / Shader Model 6.6. **Low Spec.cmd** provides a DirectX 11 / Shader Model 5 path for compatible integrated and older graphics.
- DLSS, Ray Reconstruction and Frame Generation are optional and depend on compatible NVIDIA hardware and drivers. Native temporal rendering is available without them.
- HDR is optional and requires a compatible display with Windows HDR enabled.

Choose **High**, **Medium** or **Low** in Settings. All three preserve the same combat rules.

| Preset | Scene rendering | Suggested starting point |
| --- | --- | --- |
| High | Full terrain detail, Lumen lighting, ray-traced reflections where supported, and the full effects budget | A capable modern discrete GPU; adjust resolution to your hardware |
| Medium | Lighter Lumen lighting, screen-space reflections, conventional shadows and reduced cosmetic effects | 1080p on an RTX 3060-class GPU |
| Low | Simpler lighting and soil materials, reduced texture and shadow budgets, and lighter smoke and weather | 720p on compatible integrated graphics |

The first launch estimates a preset from GPU feature support and dedicated video memory. Your saved choice is retained. These hardware classes are starting points, not claims of physical RTX 3060 or integrated-GPU testing. Performance varies with the complete system, driver and resolution.

Normal launch adapts to desktop resolution and refresh rate, using a supported mode within a 4K pixel budget on larger desktops. Display mode, resolution, frame cap, graphics, HDR, audio and language can be changed in Settings.

## Alternative launchers

Close the game before switching launchers.

- **Low Spec.cmd** starts a 720p DirectX 11 window at Low quality, with HDR and NVIDIA features disabled. Try it on integrated graphics or if the normal DirectX 12 path cannot start.
- **Safe Mode.cmd** starts a 1080p DirectX 12 SDR window with NVIDIA features disabled, for display troubleshooting on a DX12-capable GPU.

Both retain your normal display preferences and continue saving campaign progress. Return to **Play.cmd** for normal settings. Low Spec still requires a supported hardware DirectX 11 driver.

The package includes the required Microsoft x64 C++ runtime. Windows N may require Microsoft's optional Media Feature Pack. This is not a native macOS, Linux or ARM64 release, and it has not been tested on every GPU, driver and display combination.

## Updating and saves

The four matching download files contain the complete game. Extract updates into a new folder; an older version is not needed. Campaign progress remains in `%LOCALAPPDATA%\Warfare1917\Progression-v13.json`; settings and logs are under `%LOCALAPPDATA%\WarfareFront\Saved\`.

Existing victories and purchased research are preserved. Older campaign archives are backed up before migration to the branching route. Previously acknowledged dispatches are not awarded again. Archives show their difficulty and support confirmed individual or batch deletion.

Use **Check for updates** in Settings to compare your version with the latest public GitHub release and open its download page. This optional check requires internet access; it does not download or install updates automatically.

Each release supplies `START-HERE.txt` with extraction instructions and `SHA256SUMS.txt` with download checksums.

## Credits and licence

Warfare Front is an independent historical game, not an official Armor Games or Warfare 1917 / Warfare 1944 release, and is not endorsed by their rightsholders. It contains war violence and blood. Period-photo scenes are generated reconstructions, not authentic archival photographs. The strategic map and front-line sketches are illustrative, not surveyed trench maps.

Free personal play and redistribution of the complete, unmodified release with all notices are subject to `GAME-LICENSE.txt`, `THIRD-PARTY-NOTICES.md` and `Licenses/` in the extracted game folder. Third-party components retain their respective licences. Unreal Engine and NVIDIA DLSS / Reflex belong to their respective rightsholders.

The separate extractor is the unmodified 7-Zip GUI module. Its licence and corresponding source link are supplied as `7-ZIP-LICENSE.txt` and `7-ZIP-SOURCE.txt` alongside the downloads; the game retains its own licence terms.


