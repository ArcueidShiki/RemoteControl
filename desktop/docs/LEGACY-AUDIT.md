# RemoteControl audit and architecture review
Assessed 2026-10-06. Baseline: 0c51dcd. First client iteration: 98f73e2.
The Windows-only follow-up keeps that commit and wire protocol. It is not a secure Internet remote-desktop product.

## What the code does today

| Area | Evidence in this repository | Implication |
| --- | --- | --- |
| Capture and quality | RemoteCtrl/RemoteCtrl/Command.cpp, Command::SendScreen (also duplicated in CCommand): GetDC(NULL), GetDeviceCaps, BitBlt, CImage::Save(ImageFormatJPEG) | Full desktop-sized bitmap each request; lossy JPEG with no supplied quality parameters, no dirty rectangles, GPU encoder, monitor selection or resolution negotiation. Text fidelity and motion have not been validated on an actual host. |
| Resolution | The captured dimensions come from GDI; no width/height command fields | A 3840x2160 source may produce a 4K JPEG. This is not a promise of 4K streaming, DPI-correct input, multi-monitor support or any particular FPS. “2K” is ambiguous; the Windows preview uses explicit 2560x1440. |
| Transport | ClientSocket.cpp: Execute opens TCP for each command; Packet.h/cpp: head, length, command, data, additive checksum | No persistent media session, congestion adaptation, timestamps, frame IDs, encryption or peer identity. TCP retransmission and per-command setup add delay. The checksum is not authentication. The client limits packets to 16 MiB. |
| Timing | First iteration WatchDlg.cpp scheduled the next image 200 ms after completion | Delivered rate was below 5 FPS after capture, transfer and decoding. The follow-up schedules from request start, offers 2/5/10 FPS request limits and keeps one screen request outstanding. This is a pacing improvement, not a new video transport. |
| Input | Mouse.h, Command::MouseEvent, WatchDlg::PreTranslateMessage | Client sends left/right buttons and relative movement while dragging, only after its explicit opt-in. No keyboard or wheel protocol command. OS keyboard layouts, IME, secure desktop, UAC, multi-monitor and cross-DPI input remain unsupported/unverified. Client opt-in is not host-side consent. |
| Listening and privilege | Server.h: INADDR_ANY, port 20000; RemoteCtrl.cpp: IsAdmin/RunAsAdmin and ChooseBootStartUp paths; Utils.cpp: startup-directory/HKLM helpers | Server defaults to all interfaces and can request elevated startup behavior. It was not launched during this work. No firewall, startup, security-policy, VPN or system configuration was changed. |
| Authorization | Command dispatch exposes screen, files, input, lock/unlock without a pairing/authentication exchange | Do not publicly expose this server. A LAN/VPN address is not a verified identity. VPN membership alone does not supply app consent, per-feature authorization or revocation. |
| Files | DownloadFile emits a zero-size header both for an empty file and an open failure; delete/open acknowledgments lack a trustworthy outcome status | The client refuses ambiguous zero-size downloads and leaves the destination untouched. Empty files remain unsupported until versioned status codes are added. “Acknowledged” is not proof of remote success. |
| Portability | MFC windows/resources, Winsock, GDI/ATL CImage, Win32 input | The current application cannot be recompiled into a Mac client. No Mac binary was built or tested. |
| Additional host issue | SendScreen creates a stream with delete-on-release and subsequently calls GlobalFree on its handle | Potential duplicate ownership/free in host capture. Needs separate server repair and an authorized isolated host test; the server is unchanged. |

This is a targeted capability/security audit, not a claim that all server vulnerabilities have been found. Client restrictions do not secure an independently running legacy server.

## Recommendation and alternatives

For a distinct Windows/Mac product with its own experience, evaluate native capture/input and hardware encode/decode behind a portable session layer, with WebRTC as the initial media/transport candidate. Keep the present legacy adapter isolated and visibly restricted. Choose a UI toolkit after a small Windows/Mac accessibility, rendering and packaging spike. MFC polish does not provide cross-platform support.

There is no measured universal “fastest protocol.” Compare end-to-end input-to-display p50/p95, frame age, text quality, CPU/GPU use, power and bandwidth under the intended conditions before selecting an engine.

| Option | Codec / latency properties | NAT / security boundary | Maintenance / licensing consideration |
| --- | --- | --- | --- |
| Current JPEG/TCP | CPU full-frame JPEG; no motion compression or hardware path; sequential command queue can delay input | No NAT traversal or authenticated session; no encryption | Small local codebase, substantial missing safety and media work. Repository LICENSE contains GPL v2; scope of any “or later” grant needs confirmation. |
| Native platform adapters + WebRTC | Evaluate H.264 hardware path first, with negotiated alternatives based on actual endpoint support; text quality/chroma and encoder latency need measurement | ICE/STUN/TURN and encrypted media building blocks; signaling, authenticated identity binding, host consent and authorization remain product responsibilities | Most control and largest engineering burden: native media dependencies, codec/build updates, signaling and relay operations. WebRTC publishes a BSD-style license and additional IP grant; audit dependencies and codecs separately. |
| RustDesk engine/product evaluation | Official docs list VP8/VP9/AV1 software codecs and H.264/H.265 hardware paths; benchmark target hardware rather than assume advertised capability guarantees | Rendezvous/direct/relay model and encrypted sessions; evaluate its pairing/permission model against requirements | Existing cross-platform remote desktop implementation. Repository is AGPL-3.0. Do not copy/embed/relicense without a compatibility and distribution review. Can evaluate the unmodified product separately after authorization. |
| FreeRDP against RDP hosts | Mature graphics/input channels and H.264 decoding options; host capabilities and policies determine behavior | Requires an RDP-capable host and an approved authenticated route/gateway/VPN; not an arbitrary same-console Mac host solution | Apache-2.0 library/clients, including macOS build guidance. Review GPLv2 compatibility before linking to this repository. Useful alternative for a narrowly RDP-focused client. |
| Tailscale/WireGuard network overlay | Direct UDP paths typically outperform relays; overlay does not encode or optimize desktop frames | Encrypted member-node connectivity; managed identity and access policy need deliberate configuration | Separate dependency/service and operational policy. Still require app pairing, per-feature consent and revocation. No enrollment, credentials, ports or VPN changes performed. |

Official references checked 2026-10-06:
- [WebRTC peer connections, ICE, STUN/TURN and external signaling](https://webrtc.org/getting-started/peer-connections)
- [WebRTC software license and additional IP grant](https://webrtc.org/support/license)
- [RustDesk source and AGPL license](https://github.com/rustdesk/rustdesk), [official capabilities](https://rustdesk.com/docs/en/)
- [FreeRDP source and Apache license](https://github.com/FreeRDP/FreeRDP), [build options including macOS and H.264](https://github.com/FreeRDP/FreeRDP/wiki/Compilation)
- [Microsoft RDP bandwidth and adaptive graphics](https://learn.microsoft.com/en-us/azure/virtual-desktop/rdp-bandwidth)
- [Tailscale direct and relayed connections](https://tailscale.com/docs/reference/connection-types)
- [Windows Desktop Duplication API](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api)
- [Apple ScreenCaptureKit](https://developer.apple.com/documentation/screencapturekit)

## Proposed session boundaries before a larger rewrite

1. Discovery shows candidates only. On LAN, consider opt-in mDNS advertisement; no subnet sweep, automatic control or trust based on a display name/IP.
2. User selects a computer. Pair with a verified key/fingerprint or single-use code whose exchange binds to the encrypted channel. Define ownership, expiry, replay protection and revocation.
3. Host separately approves viewing and input. Default to viewing; file access, remote execution, clipboard, lock and unattended access are separate capabilities, disabled unless explicitly granted. No session-content or clipboard recording.
4. Negotiate protocol version, monitor geometry/DPI, received resolution, codec, hardware support and rate bounds. Use bounded queues and discard stale video frames. Keep ordered key/button transitions reliable; release held input on disconnect, focus loss, permission loss and reconnect.
5. Separate identity/session logic, media, input, file transfer and platform adapters. Native Windows capture can evaluate Desktop Duplication/Windows Graphics Capture; macOS capture can evaluate ScreenCaptureKit; hardware media paths can evaluate Media Foundation and VideoToolbox.
6. Use approved STUN/TURN or an approved VPN overlay for reachability. Show direct/relay path, local and peer endpoints, verified identity and measured performance. Do not infer encryption or authorization from an IP range.
7. Reconnect requires a valid session authorization and starts view-only when permission was revoked or expired. No fallback to unauthenticated legacy mode.

The concrete Windows follow-up does not implement pairing, host approval, keyboard, discovery, a VPN or video codecs. Its private-LAN opt-in is only a restriction on a legacy test client; public, CGNAT/VPN-range and link-local addresses are blocked. Loopback is the default. Private RFC1918 addresses require fresh explicit consent after disconnect. An IP range cannot prove a trusted network.

## Decisions for the next architecture stage

- Confirm whether the product must share the currently visible console on both Windows and Mac, or may use an RDP session on supported Windows hosts.
- Approve a WebRTC/platform-adapter spike versus an existing-engine/product evaluation; resolve repository and dependency licensing before integration.
- Decide attended-only versus explicitly enrolled unattended access, ownership/admin roles and revocation policy.
- Select an approved signaling/relay/VPN operator and its access policy separately. Any installation, credentials, node enrollment or network changes need authorization.

No decisions above block the bounded Windows client and loopback tests.
