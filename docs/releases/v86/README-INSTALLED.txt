# Warfare Front V86 — Installed game / 已安装游戏

## Play and installed documents

Open **Start → Warfare Front → Play**. Low Spec, Safe Mode and the uninstaller are also available in the Start menu. The game runs offline after installation.

See `ReleaseNotes.txt` in the installation folder for this version's changes. This is the same document supplied as `ReleaseNotes.md` in the GitHub release: Setup intentionally copies that Markdown attachment to the installed `.txt` filename. The two names refer to the same notes.

This support repository does not publish the game's source. GitHub's automatic Source code ZIP and TAR archives are not playable builds.

## Campaigns and language

Britain and Germany are playable. France and Russia remain **Not yet available / 暂未开放** in the four-army selector; their campaigns are closed. Existing player records are retained.

English is the default interface language. Choose **简体中文** in Settings to switch to Simplified Chinese. Primary battlefield console labels remain English in either language.

## Windows and graphics compatibility

Use **Windows 10 22H2 x64 (build 19045), fully updated, or Windows 11 x64**, with a current hardware graphics driver. Normal launch uses DirectX 12. This is not a native macOS, Linux or ARM64 build. Windows N may require Microsoft's Media Feature Pack.

A measured minimum CPU, RAM capacity and GPU performance specification has **not been established**. The Low Spec launch is a fallback, not a claim that every integrated GPU can run the game acceptably. Performance depends on the hardware, resolution and driver.

**Low Spec** uses DirectX 11, Low quality and a 720p SDR window; it requires a working hardware DirectX 11 driver. **Safe Mode** uses DirectX 12 and a 1080p SDR window. Both disable NVIDIA features for that launch and preserve campaign progress and normal display preferences. Close the game before using either Start menu entry.

DLSS, Ray Reconstruction and Frame Generation require compatible NVIDIA hardware and drivers. HDR requires a supported display with Windows HDR enabled. Settings include display mode, resolution, frame cap, three graphics presets, audio and HDR calibration.

If the taskbar remains visible after fullscreen launch, activate the game window.

## Updates and older releases

Install a newer release with its matching Setup. The in-game update check opens the latest release page; it does not download or install updates. Keep your personal save folders when removing an older portable copy.

Older releases are retained for reference in [CHANGELOG.md](https://github.com/ShoWTimETheKey/Warfare-Front/blob/main/CHANGELOG.md). For normal installation, use the [latest release](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest); historical tags have not been renamed.

## Saves and uninstalling

Saves and preferences are outside the installation folder:

- British/German campaign records: `%LOCALAPPDATA%\Warfare1917\`
- Achievements and account records: `%LOCALAPPDATA%\WarfareFront\Account\`
- Retained national campaign records: `%LOCALAPPDATA%\WarfareFront\Campaigns\`
- Preferences and diagnostics: `%LOCALAPPDATA%\WarfareFront\Saved\`

`Warfare1917` is a legacy folder name retained for compatibility. Do not rename or move it to match the game's title. Uninstalling preserves saves and preferences.

## Unsigned installer and file verification

The V86 installer is **not digitally signed**. Microsoft Defender SmartScreen may display “Windows protected your PC” or identify an unknown publisher. Keep Windows security protection enabled. If you are uncertain about the file or its origin, do not run it.

Download only from the [official repository release page](https://github.com/ShoWTimETheKey/Warfare-Front/releases/tag/v86). In PowerShell, from the folder containing the installer, inspect its checksum without running the file:

```powershell
Get-FileHash -LiteralPath '.\Warfare-Front-v86-Setup.exe' -Algorithm SHA256 | Format-List Algorithm,Hash,Path
```

Expected installer SHA-256:

```text
69e7543c6359bc7874fb3666c176850d2c8f736ca24165f30b584a446619b6dc
```

`SHA256SUMS.txt` also lists the data-file checksums. A matching hash detects corruption or a mismatch against the published value. A checksum hosted beside its download is **not independent proof of publisher identity**: an attacker able to replace both could change both. No code-signing or independent authenticity guarantee is claimed.

## Download size and free disk space

Sizes below use decimal GB (1 GB = 1,000,000,000 bytes). V86's 8 data volumes total **13.94 GB**; the installed payload is approximately **14.97 GB**. The installer is a separate small download.

For a first installation with downloads and the game on the same drive, **reserve at least 35 GB** as a conservative recommendation. **28.91 GB** is the arithmetic sum of the downloaded volumes and installed payload; it is **not a measured peak for every installation scenario**. Temporary files, copied offline volumes, existing installations kept during updates and backups can require more. Check free space on both the download/temp drive and destination drive.

The current release distributes full data volumes; no incremental patch or content-block updater is provided. Installing a newer release can require downloading its full matching data set.

## Review scope and known limits

Focused checks passed for blood lifetime, campaign-report data and storage boundaries, and report navigation in an isolated test profile. These targeted checks do not represent a complete natural campaign playthrough. They do not certify every map, balance situation, GPU, driver, display, HDR mode or Frame Generation configuration.

## Release and installed files

The GitHub release supplies `START-HERE.txt`, `ReleaseNotes.md`, `README-INSTALLED.txt`, `DOWNLOADS.json` and `SHA256SUMS.txt`, alongside Setup and the data volumes. The installed game includes `README-INSTALLED.txt`, **`ReleaseNotes.txt`**, `GAME-LICENSE.txt`, `THIRD-PARTY-NOTICES.md` and `Licenses`.

The installed `ReleaseNotes.txt` is deliberately copied from the release attachment `ReleaseNotes.md`; this is a filename mapping, not a missing document.

## Report a problem

[Open an issue](https://github.com/ShoWTimETheKey/Warfare-Front/issues/new/choose) with the game version, Windows version, GPU and driver, launch mode, steps to reproduce and a screenshot if possible. State whether the problem happens in normal launch, Low Spec or Safe Mode.

Relevant diagnostic logs are under `%LOCALAPPDATA%\WarfareFront\Saved\` (usually its `Logs` subfolder). Share only the relevant log files, with personal names and absolute user paths redacted. Do not attach entire `Account`, `Campaigns` or save folders.

## Credits and terms

Warfare Front is an independent historical game, not an official Armor Games release. Archival-style illustrations are generated reconstructions, not authentic historical photographs. The game depicts war violence and blood.

Consult the [game licence](https://github.com/ShoWTimETheKey/Warfare-Front/blob/main/LICENSE), `THIRD-PARTY-NOTICES.md` and the bundled component licences before redistributing the complete release. The installed game includes the licence as `GAME-LICENSE.txt`. Source code is not published or offered under an open-source licence.

---

# 简体中文

## 启动与安装目录文件

从 **开始 → Warfare Front → Play** 启动。开始菜单中还提供 Low Spec、Safe Mode 和卸载入口。安装后可离线游玩。

本版更新见安装目录中的 `ReleaseNotes.txt`。它与 GitHub 发行附件 `ReleaseNotes.md` 是同一份文档：安装程序会将该 Markdown 附件复制为 `.txt` 文件，两个文件名并不矛盾。

发行与支持仓库未公开游戏源码。GitHub 自动提供的 Source code ZIP 和 TAR 压缩包不是可玩版本。

## 战役与语言

英国与德国战役可玩。法国与俄国在四阵营选择页中仍标为 **Not yet available / 暂未开放**，战役尚未开放；已有玩家记录保留。

界面默认使用英文，可在设置中切换为**简体中文**。两种语言下，战斗控制台的一级标签均保留英文。

## Windows 与图形兼容性

使用**充分更新的 Windows 10 22H2 x64（内部版本 19045）或 Windows 11 x64**，并安装当前硬件图形驱动。正常启动使用 DirectX 12。本版不是 macOS、Linux 或 ARM64 原生程序；Windows N 可能需要微软 Media Feature Pack。

目前**尚未通过实测确定最低 CPU、内存容量和显卡性能要求**。Low Spec 是兼容启动选项，不代表所有核显都能流畅运行。实际表现取决于硬件、分辨率和驱动。

**Low Spec** 使用 DirectX 11、低画质和 720p SDR 窗口，需要可用的硬件 DirectX 11 驱动；**Safe Mode** 使用 DirectX 12 和 1080p SDR 窗口。两者均在该次启动中禁用 NVIDIA 特性，并保留战役进度与正常显示偏好。请先退出游戏，再从开始菜单选择。

DLSS、光线重构和帧生成需要兼容的 NVIDIA 硬件及驱动。HDR 需要受支持的显示器，并在 Windows 中启用 HDR。设置中提供显示模式、分辨率、帧率上限、三档画质、音频与 HDR 校准。

若全屏启动后任务栏仍然可见，请点击游戏窗口，使其获得焦点。

## 更新与旧版

更新时使用新版配套的安装程序。游戏内检查更新只会打开发行页面，不会自动下载或安装。移除旧的便携版文件夹时，请保留个人存档目录。

[CHANGELOG.md](https://github.com/ShoWTimETheKey/Warfare-Front/blob/main/CHANGELOG.md) 保留历次公开发行说明。正常安装请选择[最新版](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest)；既有历史标签未重新命名。

## 存档与卸载

存档及偏好位于 Windows 用户目录，不在游戏安装目录内：

- 英德战役记录：`%LOCALAPPDATA%\Warfare1917\`
- 成就及账号记录：`%LOCALAPPDATA%\WarfareFront\Account\`
- 保留的各国战役记录：`%LOCALAPPDATA%\WarfareFront\Campaigns\`
- 偏好及诊断：`%LOCALAPPDATA%\WarfareFront\Saved\`

`Warfare1917` 是为兼容而保留的旧目录名，请勿为了与游戏名称一致而将其改名或迁移。卸载程序会保留存档及偏好。

## 未签名安装程序与文件校验

V86 安装程序**尚未进行数字签名**。Microsoft Defender SmartScreen 可能显示“Windows 已保护你的电脑”或未知发布者提示。请保持 Windows 安全防护开启；若不能确认文件来源，请勿运行。

请只从[官方仓库的发行页面](https://github.com/ShoWTimETheKey/Warfare-Front/releases/tag/v86)下载。在存放安装程序的文件夹中打开 PowerShell，执行下列只读命令；命令不会启动安装程序：

```powershell
Get-FileHash -LiteralPath '.\Warfare-Front-v86-Setup.exe' -Algorithm SHA256 | Format-List Algorithm,Hash,Path
```

安装程序应有的 SHA-256：

```text
69e7543c6359bc7874fb3666c176850d2c8f736ca24165f30b584a446619b6dc
```

`SHA256SUMS.txt` 另列有各数据卷的校验值。哈希一致可排查损坏及文件与公布值不符的问题；但与下载文件同处一地的校验值**不能独立证明发布者身份**，有能力同时替换两者的人也能修改两者。本版不宣称具有代码签名或独立的身份认证保证。

## 下载量与磁盘空间

以下使用十进制 GB（1 GB = 1,000,000,000 字节）。V86 的 8 个数据卷合计 **13.94 GB**，安装后游戏数据约 **14.97 GB**；安装程序本身另计，体积较小。

首次安装且下载文件与游戏位于同一磁盘时，保守建议**至少预留 35 GB**。**28.91 GB** 只是下载数据卷与安装数据的算术和，**不是所有安装情境下实测的峰值占用**。临时文件、离线卷的复制、更新时保留旧版和备份均可能需要更多空间；请同时检查下载／临时目录所在磁盘与安装目标磁盘。

当前发行使用完整数据卷，尚未提供增量补丁或按内容分块的更新器。安装新版时可能需要重新下载该版本的完整配套数据。

## 验收范围与已知限制

本版仅通过聚焦检查，包括血液效果寿命、战役报告数据与存储边界，以及隔离测试档案中的报告导航。这些针对性检查不代表完整自然通关整场战役，也不构成对所有地图、平衡情形、显卡、驱动、显示器、HDR 模式或帧生成配置的认证。

## 发行附件与安装文件

GitHub 发行页面除安装程序及数据卷外，提供 `START-HERE.txt`、`ReleaseNotes.md`、`README-INSTALLED.txt`、`DOWNLOADS.json` 与 `SHA256SUMS.txt`。安装目录内包含 `README-INSTALLED.txt`、**`ReleaseNotes.txt`**、`GAME-LICENSE.txt`、`THIRD-PARTY-NOTICES.md` 和 `Licenses`。

安装目录的 `ReleaseNotes.txt` 由发行附件 `ReleaseNotes.md` 复制而来，是有意设置的文件名对应关系，并非遗漏。

## 反馈问题

[提交 Issue](https://github.com/ShoWTimETheKey/Warfare-Front/issues/new/choose) 时请写明游戏版本、Windows 版本、显卡与驱动、启动模式、复现步骤，并尽可能提供截图。请说明问题是在正常启动、Low Spec 还是 Safe Mode 下出现。

诊断日志位于 `%LOCALAPPDATA%\WarfareFront\Saved\`，通常在其中的 `Logs` 子目录。只提供相关日志，发送前遮去个人姓名与用户绝对路径。请勿上传整个 `Account`、`Campaigns` 或存档目录。

## 制作说明与许可

Warfare Front 是独立历史题材游戏，并非 Armor Games 官方发行版。仿古插画属于生成重构，不是真实历史照片。本作包含战争暴力与血液表现。

转发完整游戏前，请查阅[游戏许可](https://github.com/ShoWTimETheKey/Warfare-Front/blob/main/LICENSE)、`THIRD-PARTY-NOTICES.md` 及随包组件许可。安装目录中该许可的文件名为 `GAME-LICENSE.txt`。游戏源码未公开，也未授予开源许可。
