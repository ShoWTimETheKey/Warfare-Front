# Warfare Front

A First World War trench strategy game for Windows. Lead infantry and armour,
coordinate artillery and air support, and fight for ground and morale across
British and German campaigns.

## Install and play

[Download the latest Windows release](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest).

1. Download **Warfare-Front-v84.4.7-Setup.exe** from the V84.4.7 release assets and run it.
2. Choose the installation location and optional desktop shortcut. Setup downloads
   the matching V84.4.7 game data and verifies each file with SHA-256.
3. Open **Start → Warfare Front → Play**.

This release uses a Windows installer. Setup installs the complete game for your
Windows account, adds Start menu entries and registers an uninstaller. Normal
installation and play do not require administrator rights. After installation,
the game runs offline. No earlier version, Epic Games Launcher, Unreal Editor,
game account or archive tool is required.

For an **offline installation**, download setup and **every matching data file**
listed in the V84.4.7 release's `START-HERE.txt`. Keep the complete set together
with its original filenames, then run setup. Do not mix release versions. GitHub's
automatic **Source code** ZIP and TAR files are not the game.

## Campaigns and language

Britain and Germany are playable. France and Russia appear in the four-army
selector as **Not yet available / 暂未开放**; their campaigns remain closed.
Existing player records are retained.

The interface starts in English and can be switched to **简体中文** in Settings.
Battlefield console labels remain in English in either language.

## V84.4.7 changes and review scope

V84.4.7 adjusts trench exposure while firing and cover recovery during reloads,
improves MP18 close-trench assault fire, corrects local movement and machine-gun
assistant/reload placement, and fixes decoration-picker scrolling and after-action
map sheets. Physical sight lines, ranges and fortification effects remain.

The V84.4.7 desktop package passed static assembly and Windows dependency checks.
The exact final Shipping executable passed **326 focused checks** in a
**35.93-second controlled review**. Twelve actual 4K captures were reviewed and
**52 protected player files** remained unchanged. These focused checks do not
establish natural campaign balance, complete campaigns, all maps or all hardware.
No fresh relocated desktop `Play.exe` startup, HDR image-quality review or
independent generated-frame performance measurement was performed for V84.4.7.

Earlier V84.4 core FG X2 recovery after screenshot-tool focus loss passed in
exclusive fullscreen at 3840x2160 SDR with DLSS Ray Reconstruction. Its complete
screenshot-selection overlay test still failed because overlay dismissal was not
independently confirmed. A separate Editor screenshot attempt encountered a GPU
timeout and an unresponsive window. Those earlier limits were not retested here.
Installer installation and uninstall verification is a separate delivery check.

## Graphics and compatibility

Windows 11 x64 or fully updated Windows 10 22H2 x64, with a current graphics
driver. Normal play uses DirectX 12. DLSS, Ray Reconstruction and Frame Generation
require compatible NVIDIA hardware and drivers. HDR requires a supported display
with Windows HDR enabled. Settings include display mode, resolution, frame cap,
three graphics presets, audio and HDR calibration.

The Start menu also includes **Low Spec** and **Safe Mode**. Low Spec uses DirectX
11, Low quality and a 720p SDR window. Safe Mode uses DirectX 12 and a 1080p SDR
window. Both disable NVIDIA features for that launch and preserve campaign
progress and normal display preferences. Close the game before trying either.
Low Spec still requires a working hardware DirectX 11 driver.

Performance depends on hardware, resolution and driver. Windows N may require
Microsoft's Media Feature Pack. This release is not a native macOS, Linux or
ARM64 build.

## Saves, updates and uninstalling

Saves and preferences stay in your Windows user profile, outside the installation:

- British/German campaign records: `%LOCALAPPDATA%\Warfare1917\`
- Achievements and account records: `%LOCALAPPDATA%\WarfareFront\Account\`
- Retained national campaign records: `%LOCALAPPDATA%\WarfareFront\Campaigns\`
- Preferences and diagnostics: `%LOCALAPPDATA%\WarfareFront\Saved\`

Uninstalling preserves saves and preferences. Install a newer release with its
setup; the in-game update check opens the latest release page but does not install
an update. Keep your personal save folders when removing an older portable copy.

The release provides `START-HERE.txt`, `ReleaseNotes.md` and `SHA256SUMS.txt`.
The installed game includes `README-INSTALLED.txt`, `GAME-LICENSE.txt`,
`THIRD-PARTY-NOTICES.md` and the `Licenses` folder.

## Credits and terms

Warfare Front is an independent historical game, not an official Armor Games
release. Archival-style illustrations are generated reconstructions, not
authentic historical photographs. The game depicts war violence and blood.

Consult the included game licence and component notices before redistributing
the complete release.
