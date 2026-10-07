# 验证范围与证据

执行步骤见 [中文 BUILD 指南](../../BUILD.md)。测试脚本的存在不等于某一新提交已通过；应以该提交的 CI run、manifest.sourceHead 和安装包校验值为准。

## 清理基线：e88a7d7

已审查提交 `e88a7d7824d9346f92511973eedc1a7d79c1f1c0` 的
[PR CI](https://github.com/ArcueidShiki/RemoteControl/actions/runs/37619253346) 和
[push CI](https://github.com/ArcueidShiki/RemoteControl/actions/runs/37619247798) 均通过：

| 层级 | 已完成验证 |
| --- | --- |
| Node 单元 | 22 项：验证/注入、许可、取消/重复尝试、引擎摘要与环境隔离、单实例、事务存储失败和打包白名单/清单 |
| 原生开发 UI | 67 项：向导各步取消、完整中文错误、重复保存、准备/取消/重复交接、合成本地 WebRTC |
| 真实写盘失败 UI | 文件系统阻塞、重复失败保存、中文错误、取消、刷新、后续保存及进程重启，无失败编辑残留 |
| 打包应用 | ASAR 启动、实际 IPC、sandbox/contextIsolation、开发测试标志在生产包中无效、伪装引擎拒绝 |
| 多实例 | 两进程竞争、原窗口恢复、并发保存和重启后数据保留 |
| 安装器 | 一次性 Windows CI 两次实际安装、所有 payload 哈希、安装后安全 smoke、卸载保留合成设置 |
| 真实引擎探针 | 仅在一次性 Windows CI 调用已验证 RustDesk 的 --version；无连接/凭据/安装参数 |

本地另通过 66 项隔离浏览器 UI 检查及真实文件系统失败 UI 回归。浏览器 fixture 复用生产验证/存储模型，替换启动进程；它不是原生 Electron IPC 或远程会话证据。合成画布的解码尺寸/FPS、DTLS 和事件回声只证明本机 WebRTC 路径，历史采样不作为远程性能目标。

该基线安装包 SHA-256：
`8c2df4d3f39c55448f70182b4d55afd466d24cc19e950deecd99ca3f61eb3922`。
这是清理前产物，不可拿来证明清理后的提交；新清理 PR 必须重新构建并保留对应 HEAD 的结果。

## 当前清理的验收约定

清理基于上述提交，保留全部应用源码、安全测试和有效 helper；只移除已确认无依赖的旧 MFC/UML，整理文档/忽略规则及 CI 路径触发器。使用新的干净检出执行 npm ci、npm test、UI/存储回归、Windows 构建和打包/安装测试。最终完整 HEAD、CI、清单和文件哈希记录在清理 PR 与本地交接报告中，避免将自引用提交号或旧包当作新包。

`macos-stage.cjs` 同时用于 Windows 和 Mac；`MACOS-HANDOFF.md` 是分发文档白名单成员。`wizard-flow.cjs` 被 UI 测试调用，其他非 `.test.cjs` 脚本由 CI 或本地 fixture 流程调用，均不是无用文件。

## 未完成的验收

- 本地任务目录曾因 AppContainer ACL 导致 Electron sandbox 启动失败；未改 ACL 或禁用 sandbox。原生验证使用干净 Windows CI，浏览器回退不冒充原生结果。
- 早期 Mac 原生包的静态构建及启动限制见 [Mac handoff](MACOS-HANDOFF.md)。它不证明当前公共 HEAD 的 Mac GUI、安全提示或引擎验收；本次清理不访问未授权 Mac。
- 真实双机身份/加密、主机允许/拒绝/撤回、鼠标键盘/文件、实际断开/重连及 input-to-display 性能仍未完成。外部 CLI 无法提供完整会话状态。1080p60、QHD、4K 目标不是已实现的性能承诺。
- Windows 预览无 Authenticode 签名；Mac 无 Developer ID 签名或公证。不能关闭 OS 保护来替代正常发布验收。
- electron-builder 26.15.3 的历史 npm audit 记录为 sprintf-js 链上的 8 项 moderate 构建依赖问题；不进入运行时 ASAR。清理不强制升级或覆盖锁定依赖，最新 audit 结果随新检出验证记录。

任何真实引擎安装/配对、VPN/网络/权限变更、发布或 PR 合并仍需对应授权。2026-10-07 核对：PR #67 已于 15:20:58 UTC 在本清理任务之外合并；PR #68–70 尚未合并，继续等待明确的合并授权。#67 的外部合并不代表用户批准合并其他 PR。
