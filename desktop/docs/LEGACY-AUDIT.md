# 旧 MFC 服务风险与历史

2026-10-07 清理后，当前源码不再包含 `RemoteCtrl/`、旧 `.sln/.vcxproj` 和 `RemoteCtrl System Design.uml`。新版 `desktop/src`、构建脚本、测试和 CI 对这些文件没有运行依赖。旧代码仍可从 Git 历史查看；没有改写历史或删除已有分支。

## 为什么移除旧服务

这是对旧实现的定向审计结论，不代表已穷举全部漏洞：

- 旧协议没有配对/身份验证、传输加密或主机逐功能授权；数据包的加法校验和不是认证。屏幕、输入、文件和锁屏指令直接进入命令分发。
- 监听默认绑定所有接口、端口 20000；存在请求管理员身份及开机启动的路径。不能把旧服务暴露到公网，也不能依靠 LAN/VPN 地址替代授权。
- GDI 全帧 JPEG 和逐命令 TCP 不提供现代视频会话、硬件编码或性能保证。旧 UI 的客户端勾选不能代替主机同意；输入、DPI、多显示器及文件操作结果也有缺失/歧义。
- 客户端限流、取消和参数约束无法补齐服务器的认证缺口。新版不启动旧服务，也没有失败后退回旧协议的路径。

具体证据位于历史 `Server.h`、`RemoteCtrl.cpp`、`Command.cpp`、`Packet.*` 和客户端 `ClientSocket.cpp`、`WatchDlg.cpp`。原始逐项审计保留在下方完整提交的文档中。

## 固定历史记录

| 提交 | 内容 |
| --- | --- |
| [0c51dcd653f0365739d8e99c902aecdd72f29c8d](https://github.com/ArcueidShiki/RemoteControl/tree/0c51dcd653f0365739d8e99c902aecdd72f29c8d/RemoteCtrl) | 旧 MFC 基线 |
| `98f73e2d3c2f2e94ba0dff9a0bc1530283921d77` | 保留在本地 Git 历史/原分支的客户端简化迭代 |
| `3df6d57aba1b06126485377ed6d24e203119bd89` | 保留在本地 Git 历史的受限 Windows 查看器与审计 checkpoint |
| [e88a7d7824d9346f92511973eedc1a7d79c1f1c0](https://github.com/ArcueidShiki/RemoteControl/tree/e88a7d7824d9346f92511973eedc1a7d79c1f1c0) | 清理前已审查的新界面/存储修复；仍包含旧源码和完整历史审计 |

仅查看历史，不执行旧服务。在包含对应对象的检出中：

```powershell
git show e88a7d7824d9346f92511973eedc1a7d79c1f1c0:desktop/docs/LEGACY-AUDIT.md
git show e88a7d7824d9346f92511973eedc1a7d79c1f1c0:RemoteCtrl/RemoteCtrl/Server.h
```

清理只删除已跟踪的旧项目文件。删除前已确认旧目录无未跟踪或忽略的用户文件；其他 profile、构建产物和审查证据未清空。未来若发现未跟踪历史文件，应先归档到独立目录，核对可恢复性，不能用 `git clean -fdx` 清扫用户数据。

当前设计和双机验收要求以 [架构说明](CURRENT-ARCHITECTURE.md)、[验证记录](VALIDATION.md) 与 [构建指南](../../BUILD.md) 为准。许可证及运行时 notices 保留；清理不是对旧服务安全性或新引擎整合完成度的背书。
