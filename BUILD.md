# RemoteControl 中文构建、Debug、测试与安装指南

当前应用入口是 `desktop/`，版本与精确依赖见 [package.json](desktop/package.json) 和 [package-lock.json](desktop/package-lock.json)。清理分支基于已审查的 `e88a7d7`；旧 MFC 客户端/服务器、`.sln/.vcxproj` 和 UML 已移出当前源码，历史仍可查看。

**不使用 Visual Studio Publish。** 新版是 Electron + JavaScript，没有旧 MFC 的 Debug/Release 配置，也没有 `dotnet publish`、MSBuild 或额外前端编译步骤。`npm start` 用开发 Electron 运行源码；Windows 安装包由仓库脚本调用 Electron Packager、electron-builder 和 NSIS 生成。VS/VS Code 可以作编辑器，Publish 按钮不是本项目安装器入口。

## 1. 准备环境与正确检出

- Git 和 Node.js **22.12+**（建议与 CI 一致使用 Node 22 x64；npm 随 Node 提供）。
- Windows 构建必须使用 **Windows x64 + x64 Node**。当前无原生 npm 扩展，无需安装旧 MFC/C++ 工作负载来构建新版。
- 使用正常授权的开发目录及网络，允许 npm/Electron/构建工具获取锁定依赖。不要为了下载、启动或调试关闭系统保护。
- 本地原生 GUI 测试使用私有桌面；共享桌面需要先协调。应用的自动化测试会打开/恢复自己的窗口。
- Mac 构建需已授权的、架构匹配的原生 Mac；详见第 7 节。

确认环境：

```powershell
git --version
node --version
npm --version
node -p "process.platform + ' ' + process.arch"
```

Windows 打包的最后一行必须为 `win32 x64`。仅有 ZIP/复制出的源码不够：打包脚本还需要 Git 提交对象、锁文件和完整 HEAD。

没有检出时，可以从已确认地址取得此清理分支（草稿 PR 阶段，不代表主分支已合并）：

```powershell
git clone --branch codex/desktop-cleanup-guide https://github.com/ArcueidShiki/RemoteControl.git RemoteControl-clean
cd RemoteControl-clean
git rev-parse HEAD
git status --short
```

核对输出与待审 PR 的完整 HEAD。后续发布应使用审查批准的提交；不要把 `main`、旧 PR 的包或其他本地工作树当作当前版本。已有工作区先检查变更，避免覆盖未提交内容。需要验证时可另建干净 clone；不要用 `git clean -fdx` 或强制 reset 清空用户文件。

## 2. 安装依赖、测试和启动

在仓库根目录开始：

```powershell
cd desktop
npm ci
npm test
npm start
```

逐条运行；任一步失败就停止检查。PowerShell 批量脚本应在每个原生命令后检查 `$LASTEXITCODE`，不能假定分行命令会自动遇错停止。

`npm ci` 安装 lockfile 的固定开发依赖，并会替换**当前 desktop 下**已有的 node_modules；不更新 lockfile。不要用全局 Electron/builder、`npm update` 或 `npm audit fix --force` 代替。构建用 Electron 版本由 package.json 固定，首次安装/运行可能需要获取对应 runtime。

`npm start` 打开工作区。首次向导最终一步才保存电脑，不会自动连接；连接许可初始不勾选。当前只有通过内容摘要校验的 Windows RustDesk 程序才可交接。没准备引擎也能查看界面与保存设备。不要为方便测试删除校验、绕过许可或运行旧服务器。

普通启动会使用 Electron 默认 userData（Windows 通常为 `%APPDATA%\remotecontrol-desktop`），其中 devices.json 保存设备元数据与程序路径。开发调试建议在独立终端指定隔离 profile：

```powershell
$env:REMOTECONTROL_PROFILE = Join-Path $PWD '.userdata/manual-review'
npm start
```

此变量只影响当前终端及其子进程。关闭应用后关闭该终端，或 `Remove-Item Env:REMOTECONTROL_PROFILE` 恢复默认。每个 profile 有单实例锁；再次运行同一 profile 会恢复已有窗口，不是启动新的会话。不要手工并行修改 devices.json；失败的设置写入不会提交到内存或后续保存。

## 3. Electron Debug：主进程与渲染进程分别调试

请使用开发源码与独立 profile。安装后的生产包关闭 renderer DevTools；不要为 Debug 修改生产包安全配置。

### 主进程：启动、IPC、设置存储与引擎验证

在 `desktop` 的 PowerShell 中：

```powershell
$env:REMOTECONTROL_PROFILE = Join-Path $PWD '.userdata/main-debug'
.\node_modules\.bin\electron.cmd --inspect-brk=127.0.0.1:9229 .
```

`--inspect-brk` 暂停在主进程入口，窗口尚未出现是正常的。在本机 Chrome 打开 `chrome://inspect`，Configure 中确认 `127.0.0.1:9229`，找到 Electron/Node target 点 inspect，再继续执行（F8）。端口被占用则换一个本机端口，并同步修改调试器配置。不要绑定 `0.0.0.0`、开放防火墙或将调试端口暴露给其他机器。

在 Sources 中给 `src/main.cjs`（创建窗口、IPC handler）、`src/core.cjs`（设置事务、取消/交接）、`src/engine.cjs`（内容验证）设断点。已有同 profile 进程要先正常关闭，否则新启动会因单实例锁退出。只需要附加已开始执行的启动过程时，可将 `--inspect-brk` 换为 `--inspect`。机制参见 [Electron 主进程调试](https://www.electronjs.org/docs/latest/tutorial/debugging-main-process)。

### 渲染进程：向导、DOM、样式与事件

主进程调试器与窗口内 DevTools 是不同目标：

1. 在主进程 `src/main.cjs` 的 `await window.loadURL('workspace://app/index.html')` **之后**、`windowReady = true` 处设断点，继续到该处。
2. 在该断点的 **main 作用域 Console** 执行 `window.webContents.openDevTools({ mode: 'detach' })`，然后继续运行。这里的 window 是主进程 BrowserWindow，不是浏览器 DOM window；不要把这条命令粘贴到 renderer Console。
3. 在打开的 renderer DevTools 里，Sources 查找 `workspace://app/app.js`，Elements 查看 index.html/style.css，Console 查看界面异常及 IPC 返回。界面 JS 的断点不调试 main.cjs。
4. 修改 app.js/style.css 后重新加载开发窗口；修改 main.cjs/preload.cjs 后关闭并重新启动应用。没有热更新服务器。

renderer 没有 Node `require` 是预期安全行为；无需开启 nodeIntegration、关闭 contextIsolation/sandbox 或增加任意文件/命令 IPC。打开 DevTools 的 API 见 [Electron application debugging](https://www.electronjs.org/docs/latest/tutorial/application-debugging)。如果调试 Console 不在正确调用栈作用域，重新选择 main.cjs 的上述断点帧，勿通过改安全选项规避。

开发日志可在专用终端临时设置 `$env:ELECTRON_ENABLE_LOGGING='1'` 后运行；日志可能包含本地路径，不要提交个人日志。完成后关闭调试窗口和进程。

### 本地 Electron 启动受限时

本任务环境曾因继承的 AppContainer ACL 导致 sandbox 启动失败。不能通过 `--no-sandbox`、改 ACL、停安全软件解决本项目验收。使用正常授权的开发机或 Windows CI 做原生验证；第 4 节的 Chrome fixture 只辅助看界面，不证明 Electron IPC/生产包已工作。

## 4. 测试命令与用途

所有命令在 `desktop` 运行。未列出实际引擎参数的常规测试均不会建立远程连接。

| 命令/脚本 | 前提与验证范围 |
| --- | --- |
| `npm test` | Node 单元与模型测试：参数/注入、许可、取消/重复、身份摘要、最小环境、单实例门禁、真实写盘失败事务、共享打包白名单及 ASAR/ZIP 清单 |
| `npm run test:ui` | 原生开发 Electron；fake engine + 本地合成 WebRTC。向导取消/返回/错误、重复保存、准备取消/重复交接、真实 DOM 截图 |
| `node tests/wizard-storage.cjs` | 原生隔离 profile；真实 fs 写入阻塞、重复失败、取消/刷新、后续保存、应用重启，无失败数据泄漏 |
| `npm run test:packaged` | 先构建 Windows 包；实际 ASAR/IPC、sandbox/isolation、伪装引擎拒绝、生产包忽略 REMOTECONTROL_TEST |
| `npm run test:instances` | 先构建 Windows 包；真实两个进程竞争、恢复原窗口、保存与重启。会操作自己的窗口，放在私有桌面或 CI |
| `node tests/trusted-engine.cjs ABSOLUTE_PATH` | 可选；仅对已经授权取得的指定 Windows 官方程序做摘要/保存路径/改写拒绝和捕获参数检查。默认不执行引擎，不会替你下载/安装 |
| `tests/installer-smoke.cjs` | **仅一次性 GitHub-hosted Windows CI**：实际安装两次、payload 校验、安装后 smoke、卸载和设置保留。会涉及测试用户 HKCU/开始菜单 |
| `tests/trusted-engine.cjs ... --probe-version` | **仅同类一次性 CI**：真实运行 --version。上游 portable wrapper 即使查询版本也可能解包到用户 profile；不是会话测试 |

后两项已由 [Windows CI](.github/workflows/desktop.yml) 按顺序执行。不要在日常桌面伪造 `GITHUB_ACTIONS` 或 `RUNNER_ENVIRONMENT` 来越过限制，也不要直接复制 CI 安装/卸载脚本到用户机器。

有效 helper 不能按文件名后缀删除：`wizard-flow.cjs` 被 electron-ui 调用；browser-review/wizard-errors 支持浏览器回退；packaged、storage、instances、installer 和 trusted-engine 是独立入口。它们不应全部改成 `*.test.cjs` 混入 Node 单元测试。

### Chrome 界面回退（可选）

需要本机现有 Chrome。在终端 A：

```powershell
node tests/browser-review.cjs
```

保持进程运行，记录其输出的 `http://127.0.0.1:端口`。终端 B 同样进入 desktop，将实际端口填入变量：

```powershell
$env:REMOTECONTROL_BROWSER_URL = Read-Host '粘贴终端 A 的 loopback URL'
node tests/electron-ui.cjs
node tests/wizard-errors.cjs
Remove-Item Env:REMOTECONTROL_BROWSER_URL
$env:REMOTECONTROL_STORAGE_BROWSER = '1'
node tests/wizard-storage.cjs
Remove-Item Env:REMOTECONTROL_STORAGE_BROWSER
```

使用 headless Chrome 且保留 sandbox；loopback fixture 的启动器为假进程，存储与校验是生产模型。wizard-errors 模拟 HTTP 失败；wizard-storage 使用真实文件系统阻塞，两者不可互相替代。测试 profile 位于 .userdata，截图及 WebRTC 诊断在 output/playwright。结束后在终端 A 按 Ctrl+C。不要把这些合成截图/FPS 当作实际远控或生产 IPC 验收。

## 5. 构建 Windows setup（不会安装）

在 **Windows x64** 的 desktop 中，先完成源码变更审查与提交，再运行：

```powershell
git status --short
$revision = (git rev-parse HEAD).Trim()
npm ci
if ($LASTEXITCODE -ne 0) { throw 'npm ci failed' }
npm test
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
npm run package:win -- $revision
if ($LASTEXITCODE -ne 0) { throw 'Packaging failed' }
```

`git status --short` 应无输出。脚本要求完整 40 位 SHA 且等于当前 HEAD，拒绝未提交/未跟踪源码和依赖版本不符。不要传短 SHA、分支名或字面的 FULLHEAD。忽略的用户数据不会成为打包输入：脚本从指定提交读取明确白名单的**普通 Git blob**，不复制整个工作树或 node_modules。新增运行时依赖需独立审查，不会被悄悄漏装。

构建调用链：

`package:win → scripts/build-windows.cjs → scripts/macos-stage.cjs → Electron Packager → electron-builder / NSIS`。

`macos-stage.cjs` 名称带 macos，但 Windows 同样依赖它；不能删。`docs/MACOS-HANDOFF.md` 仍在分发文档白名单，项目 LICENSE、Electron LICENSE、LICENSES.chromium.html 和源码来源记录必须保留。构建核对完整 ASAR 清单及安装器生成前后的 payload 哈希，不会自动发现本地证书、安装器 hooks 或未跟踪资源。

成功产物（VERSION 来自 package.json，HEAD12 为 revision 前 12 位）：

```text
desktop/dist/
  RemoteControl-win32-x64/
    RemoteControl.exe
    resources/app.asar
    READ-ME.md
    MACOS-HANDOFF.md
    SOURCE-REFERENCE.json
    PREVIEW-BOUNDARIES.txt
    LICENSE
    LICENSES.chromium.html
    PROJECT-LICENSE.txt
    ... Electron 运行时文件
  RemoteControl-VERSION-windows-x64-HEAD12-setup.exe
  RemoteControl-VERSION-windows-x64-HEAD12-setup.exe.sha256
  RemoteControl-VERSION-windows-x64-HEAD12-setup.exe.json
```

应用目录是解包后的验证目标；setup.exe 才是 NSIS 安装包，不要只复制其中 RemoteControl.exe。JSON 记录完整 sourceHead、平台/架构、版本、安装器 SHA-256 和 ASAR/payload 清单。构建会替换 dist 内固定应用目录以及同提交的同名包；保留旧证据时先复制到忽略的 artifacts 目录。

例如在 desktop 中核对本次产物：

```powershell
$version = (Get-Content package.json -Raw | ConvertFrom-Json).version
$setup = Join-Path $PWD ("dist/RemoteControl-" + $version + "-windows-x64-" + $revision.Substring(0,12) + "-setup.exe")
$manifest = Get-Content -LiteralPath ($setup + '.json') -Raw | ConvertFrom-Json
if ($manifest.sourceHead -ne $revision) { throw 'Source HEAD mismatch' }
$actual = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $manifest.installer.sha256) { throw 'Installer hash mismatch' }
Get-Content -LiteralPath ($setup + '.sha256')
Get-AuthenticodeSignature -LiteralPath $setup
```

再在私有桌面或 CI 运行 `npm run test:packaged` 和 `npm run test:instances`。Windows 构建脚本本身不会代跑全部 UI/安装测试；构建成功不等于测试通过。不同构建机器/时间的 NSIS 文件可有不同 SHA；必须报告当前实际文件，不拿旧 CI 哈希替代。

## 6. setup 安装与交付

本指南区分构建与安装。只有用户批准安装该精确版本时，才在指定账户正常运行核对过的 setup.exe。它是 **per-user、one-click NSIS** 预览，无管理员安装模式；创建开始菜单 RemoteControl 和正常卸载入口，不创建桌面快捷方式、不自动启动、不创建服务或开机任务。通常安装到当前用户 LocalAppData 的 Programs 下，以实际安装结果为准。

安装结束从开始菜单启动。正常卸载保留 userData 设置；需要重新开始时另选调试 profile，不要顺手删除用户资料。测试自动卸载只能按第 4 节放在一次性 CI。

当前预览 **没有 Authenticode 签名**，脚本也不读取签名身份。若 Windows 阻止，停在正常安全流程，不能教用户关闭保护。正式分发还需要已批准的签名、验证和发布流程。构建使用 `publish: never`；生成 setup 不等于上传 release、部署或获得 PR 合并批准。项目不包含自动更新发布流程。

交付记录至少包含：完整 sourceHead、CI 链接、setup 文件名/字节数/SHA-256、对应 .sha256/.json、是否安装及位置、原生测试结果与尚未验证范围。CI 安装器 artifact 通常保留 14 天、UI 证据 7 天，不是永久发布页。

## 7. Mac 构建条件与产物

只在获授权的匹配架构 Mac 上使用仓库同一审查提交与 Git/Node 22.12+，确认系统有脚本使用的 plutil、lipo、ditto、unzip、codesign。不要在 Windows 上宣称已完成 Mac 原生验证，也不要为本指南自动安装/切换 Xcode 或修改系统权限。

Apple Silicon 原生 arm64，终端从仓库根目录开始：

```sh
cd desktop
npm ci
npm test
git status --short
revision=$(git rev-parse HEAD)
node scripts/build-macos.cjs "$revision" arm64
```

Intel 原生 Mac 将末尾参数改为 `x64`；脚本要求 requested arch 与当前 Node 的 process.arch 一致。这里同样要求干净提交及完整 SHA；构建脚本会再次跑 Node 测试。**不要运行旧的 `npm run package:mac`**：它故意拒绝旧的整目录复制打包方式，不是待绕过的错误。

产物位于 desktop/dist：`RemoteControl-darwin-arm64/RemoteControl.app`（或 x64），以及 `RemoteControl-VERSION-macos-ARCH-HEAD12-unsigned-preview.zip`、同名 .json 和 .zip.sha256。它是 app/ZIP 预览，**没有 DMG/PKG 安装器**。脚本检查 bundle/架构、完整 ASAR、ZIP 条目及解压后文件/权限/链接清单。

没有 Developer ID 签名或公证。上游/ad-hoc framework 签名不等于发行者身份或 Gatekeeper 通过；不能删 quarantine、关闭 Gatekeeper、改 TCC 或临时重签来绕过失败。详细历史证据和当前 Mac 引擎校验门禁见 [MACOS-HANDOFF](desktop/docs/MACOS-HANDOFF.md)。现有 packaged/instances 测试入口默认是 Windows EXE，不能直接当作 Mac 通过证据。

## 8. 常见问题与当前能力边界

| 情况 | 处理 |
| --- | --- |
| HEAD 不符/dirty checkout | 核对分支与完整 SHA，审查并提交源码；用户未跟踪文件先可恢复归档，不强制清空 |
| 依赖版本不符/缺 Electron | 在正确 desktop 执行 npm ci，核对锁文件与正常网络；不要复制其他项目 node_modules |
| 找不到入口/想用 VS Publish | 回到 desktop/npm start 或上述 package:win；旧 MFC 工程已退休 |
| inspect 没窗口 | inspect-brk 正暂停，连接本机调试器后继续；也检查同 profile 是否已有进程 |
| 保存失败 | 设置保持原值；检查空间/写入权限后重试或取消，不编辑/删除用户原始数据来“修复” |
| 选了 RustDesk 仍不允许连接 | 检查受支持的版本、标准文件名及摘要；不能扩大白名单或关闭身份校验 |
| 原生启动/OS 签名阻止 | 保留错误与环境信息，走正常 runtime/签名验收；浏览器回退只验证界面 |
| npm audit 有告警 | 记录当前锁文件和构建依赖链；独立评估升级，不能用强制升级换取“全绿” |

现有产品是 external RustDesk launcher：RustDesk 未捆绑，窗口中的“交接/状态未知”不代表已鉴权或已连接。实际会话许可、身份确认、加密与断开在引擎中处理。Mac 引擎尚未通过身份审查，保持拒绝启动；没有旧协议回退。

尚未完成两台授权设备的安全/功能/性能验收：错误凭据拒绝、主机同意/拒绝/撤回、实际输入与断开重连，以及统一外部 input-to-display 测量下的 1080p60/QHD/4K。VPN 连通不证明应用会话安全，合成本机 FPS 不证明远程性能。安装/配对基准软件、网络/权限变更需要另行授权。

[当前架构](desktop/docs/CURRENT-ARCHITECTURE.md) · [验证记录](desktop/docs/VALIDATION.md) · [旧服务风险与历史](desktop/docs/LEGACY-AUDIT.md) · [向导与安装器设计](desktop/docs/WIZARD-INSTALLER.md)。
