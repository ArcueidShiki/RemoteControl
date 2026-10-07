# RemoteControl

当前可维护的应用位于 `desktop/`：Electron 桌面界面、设备列表、首次连接向导，以及独立 RustDesk 窗口的启动入口。旧 MFC 客户端、旧服务器、Visual Studio 工程和 UML 已从当前源码移除；Git 历史与已有分支保留。

**从 [中文构建、Debug、测试与安装包指南](BUILD.md) 开始。** 新版不使用旧 `.sln/.vcxproj`，也不通过 Visual Studio 的 Publish 生成安装包。

## 快速开始

准备 Git 和 Node.js 22.12+（CI 使用 Node 22），在仓库根目录运行：

```powershell
cd desktop
npm ci
npm test
npm start
```

这会打开新版工作区。保存设备不会自动连接；实际会话仍在用户另行取得的、通过校验的 RustDesk 中完成。

## 项目结构

| 路径 | 用途 |
| --- | --- |
| [BUILD.md](BUILD.md) | Windows/macOS 构建条件、主进程与界面调试、测试范围、setup 产物及排错 |
| [desktop/src](desktop/src) | 当前应用与引擎身份、权限和设置存储边界 |
| [desktop/tests](desktop/tests) | 单元、原生 UI、打包、安全、安装器与测试辅助程序 |
| [desktop/scripts](desktop/scripts) | 从固定 Git 提交构建；Windows/Mac 共享白名单和清单校验 |
| [架构说明](desktop/docs/CURRENT-ARCHITECTURE.md) | 当前能力与安全边界 |
| [验证记录](desktop/docs/VALIDATION.md) | 有版本依据的结果及未完成验收 |
| [旧服务风险与历史](desktop/docs/LEGACY-AUDIT.md) | 删除原因、历史提交与只读查看方式 |

本仓库许可证 [LICENSE](LICENSE) 保持不变。Electron/Chromium 及项目 notices 随包保留。RustDesk 未捆绑，其独立许可与未来整合方式需要单独审查。

当前应用不能确认外部会话的鉴权、加密、直连/中继状态或实际断开；不能宣称已完成双机安全与性能验收。Mac 引擎仍被校验门禁阻止。没有旧服务器回退，也不会自动安装引擎、修改 VPN、防火墙、开机启动或主机权限。

开发变更先在独立分支验证，再通过草稿 PR 审查。清理历史源码不等于授权合并既有 PR、发布或部署。
