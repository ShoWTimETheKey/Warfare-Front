# Warfare Front

一战西线战壕策略游戏 · Windows 独立运行版。

**[下载最新完整游戏](https://github.com/ShoWTimETheKey/Warfare-Front/releases/latest)**

在 Releases 的 **Assets** 中下载 `Warfare-Front-v33.1-Windows-x64.zip`，完整解压后双击 `WarfareFront.exe` 或 `Play.cmd`。无需安装 Unreal Engine、Epic Games Launcher，也无需登录游戏账号。请勿下载 GitHub 自动生成的 `Source code` 压缩包：本仓库用于发布成品，不包含开发工程。

## 运行要求

- Windows 10 22H2 / Windows 11，64 位。
- 支持 DirectX 12 / Shader Model 6.6 的现代显卡及其当前稳定驱动；不提供 DX11、32 位、macOS 或 Linux 原生版本。
- DLSS、光线重构和帧生成按显卡与驱动支持情况启用；不支持时自动回退。HDR 为可选功能，需要 Windows 和显示器均支持并启用 HDR。
- 初次启动跟随桌面分辨率；超高分辨率桌面优先选择不超过 4K 像素预算的受支持模式。可在游戏内调整画质、分辨率、帧率上限和音量。

首次启动可能需要编译着色器，请稍候。若显示配置导致黑屏或无法正常显示，关闭游戏后双击 `Safe Mode.cmd`，以 1080p 窗口和原生渲染启动；该入口不保存显示设置，正常战役进度仍可保存。

## 本次发布

V33.1 保留 V33 的最新游戏内容，改用 Shipping 构建并补齐随包 x64 VC++ 运行库，清除调试组件、开发工程、历史报告和个人存档。游戏及其必要的第三方许可随完整压缩包分发。

发布检查涵盖独立目录解压启动、主菜单、战役载入、招募、随包 DLL 依赖与禁用 NVIDIA 功能后的原生回退。检查在开发机完成；尚未取得其他实体 AMD / Intel 显卡电脑的实测结果，不对所有硬件配置作保证。

界面主要为中文，战场包含英语和德语语音。游戏包含战争暴力及血腥表现。

## 许可与鸣谢

本作是独立历史题材作品，并非 Armor Games 或《Warfare 1917》《Warfare 1944》的官方发行版，亦未获其背书。仅发布已封装游戏，不提供原作代码或资源。

个人免费游玩；允许保留全部许可与署名后免费转发完整、未修改的发行包。详细条款及各素材来源见压缩包内 `GAME-LICENSE.txt`、`THIRD-PARTY-NOTICES.md` 与 `Licenses/`。第三方组件各自的许可优先适用。

Unreal® Engine 与 NVIDIA DLSS / Reflex 的商标和软件权利属于各自权利人。
