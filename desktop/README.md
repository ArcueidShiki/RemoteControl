# RemoteControl desktop · 0.3 预览

这是当前应用入口。中文工作区包含设备列表、四步首次连接向导及设置/帮助；本地合成诊断收在高级设置中。实际远控、文件传输和断开仍由独立 RustDesk 窗口处理。

完整中文步骤在源码仓库根目录的 BUILD.md：依赖安装、启动、Electron 主进程/renderer Debug、各层测试、Windows setup 与 Mac 构建。若正在查看安装目录的 READ-ME.md，可通过同目录 SOURCE-REFERENCE.json 的 sourceUrl 找到对应源码及 BUILD.md。

## 本地开发

从源码仓库的 desktop 目录运行：

```powershell
npm ci
npm test
npm start
```

需要 Git、Node.js 22.12+；CI 固定使用 Node 22。Electron、Packager、electron-builder 和 Playwright 由 package-lock.json 固定，勿另装全局版本或通过自动升级改变依赖。没有单独的 TypeScript/前端编译步骤，也不需要旧 MFC 工程。

Windows x64 安装包从干净、已提交的完整 HEAD 构建：

```powershell
$revision = (git rev-parse HEAD).Trim()
npm run package:win -- $revision
```

输出在 dist：RemoteControl-win32-x64 应用目录，以及带版本和 HEAD 前 12 位的 setup.exe、.sha256、.json。构建不安装、不自动启动、不发布。安装器为未签名的 per-user NSIS 预览；正常安装提供开始菜单入口，正常卸载保留设置。

Mac 只能在获授权、架构匹配的 Mac 上运行 scripts/build-macos.cjs FULL_40_CHARACTER_HEAD arm64 或 x64；详见源码中的 docs/MACOS-HANDOFF.md。package:mac 已被安全封禁，不能用于构建。当前不提供 Developer ID 签名、公证或已验收的 Mac 安装器。

## 使用与边界

保存电脑仅写入名称、数字 RustDesk ID、可选 IP 备注及程序路径，不写密码或剪贴板。IP 是备注，不证明远端身份或可达性；本地网卡信息也不能证明 VPN 已认证或会话加密。许可默认不勾选，取消准备不会启动引擎；交接后应在 RustDesk 内结束会话，再显式确认开始下一次尝试。

仅允许已审查的 RustDesk 1.5.0 Windows x64 portable 文件，文件名为 rustdesk-1.5.0-x86_64.exe 或 rustdesk.exe。已固定 SHA-256：
8555777215510d83d2d61c9dc984e4fcc838bd7e79f9d18a42585431f5e8bb47。
每次恢复路径和启动前重新验证内容；保存路径不等于信任。其他版本、伪装文件和未经审查的 Mac 引擎会被阻止。程序不下载、安装或配置 RustDesk，也不传密码、直连 IP、安装或提权参数。

渲染进程启用 sandbox/contextIsolation，关闭 Node integration，IPC 校验发送者，仅加载本地资源。每个设置 profile 由单实例锁管理；候选设置成功写盘后才更新内存，失败编辑不会污染后续保存。引擎使用固定参数数组、shell:false、受限环境及专用工作目录。

本地 WebRTC 诊断只传合成画布和事件种类，不捕获桌面、不控制 OS、不录制会话。其 FPS/往返值不是 RustDesk、WAN 或远程 input-to-display 性能。双机身份/加密、主机授权/撤回、输入与真实断开/重连、1080p60/QHD/4K 仍待独立验收。

## 许可证与验证

项目 LICENSE 不变；安装目录保留 Electron LICENSE、LICENSES.chromium.html 和 PROJECT-LICENSE.txt。RustDesk 为独立 AGPL-3.0 项目，未复制、链接或捆绑引擎；独立进程并非未来所有分发方式的许可结论。

测试范围和已验证提交见源码 docs/VALIDATION.md；架构见 docs/CURRENT-ARCHITECTURE.md。安装/卸载自动化与真实引擎 --version 探针仅在一次性 GitHub-hosted Windows CI 运行，不在日常桌面伪造 CI 环境变量运行。安全限制不会为“能启动”而关闭。
