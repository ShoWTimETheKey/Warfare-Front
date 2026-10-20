# Warfare Front V86

- Completed campaigns now open a detailed report covering the recorded battles,
  both armies, unit losses, armour destruction and fire-support results. Earlier
  saves retain their available totals without inventing missing battle records.
- Corpse blood settles and dries without repeatedly restarting its flow as
  nearby effects appear.
- Improved armour movement through occupied trench areas.
- The German creeping-barrage research photograph has been refined to match the
  wear, tonal range and grain of the other unresearched technology illustrations.

## Checks

Focused checks passed for blood lifetime, campaign-report data and storage
boundaries, and report navigation in an isolated test profile. These targeted
checks do not represent a complete natural campaign playthrough.

French and Russian campaigns remain closed. Existing player records are retained.

---

# Installation, security and review limits / 安装、安全与验收范围

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

---

# 简体中文

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
