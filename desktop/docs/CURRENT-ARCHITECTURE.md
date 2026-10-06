# Current architecture and acceptance boundary

Decision: Electron shared UI and Node session/device models, with an external, unmodified RustDesk application as the remote-desktop engine.

Why: the repository is Windows/MFC only; Rust/Cargo and a cross-platform native UI toolchain are not installed here. Node is available, Electron supplies one Windows/macOS interface, and RustDesk already maintains desktop media/input/file-transfer and cross-network behavior. This avoids inventing a remote-control security protocol in a UI rewrite. Electron carries a larger runtime/memory footprint; no “lightest” or “fastest” claim is made.

This first milestone is an explicit separate-process integration. It is not an embedded engine, not a reimplementation of RustDesk, and not the completed professional remote-desktop target. The old MFC work is preserved on separate local commits 98f73e2 and 3df6d57; this PR starts from the original main branch and adds the new desktop tree only.

## Security boundaries

The renderer loads packaged local files through a restricted custom protocol. It has context isolation and Chromium sandboxing enabled, no Node integration, a restrictive CSP, no arbitrary shell/file bridge and no navigation/popups. IPC accepts only the app's main frame. Devices and settings contain no credentials. String fields render through textContent.

The RustDesk adapter accepts numeric IDs and fixed desktop/file-transfer commands. It uses argument arrays with shell:false and requires fresh explicit user consent. It never supplies a password, changes RustDesk configuration, starts its installer, alters host permissions, enrolls a VPN node or opens a firewall rule. Direct-IP connections are excluded because an IP alone is not authenticated identity.

The host's actual view/control/file permissions and encrypted-session verification remain in RustDesk. The supported CLI was inspected at the official 1.5.0 tag: src/core_main.rs accepts --connect and --file-transfer. It does not provide a force-view-only switch, live session state, metrics subscription or targeted disconnect API. The UI therefore says “Continue in RustDesk” and “handoff,” never “connected.” The user must end a session in RustDesk before clearing the handoff. Closing the workspace does not kill a potentially shared RustDesk process.

## License and component review

- Repository LICENSE: GPL v2 text, unchanged. Confirm exact grant and compatibility before a combined derivative distribution.
- Electron: MIT; its runtime and Chromium notices are retained in the package. Dependencies are pinned in package-lock.json.
- RustDesk: AGPL-3.0, separately obtained. No engine source is copied/linked and no RustDesk binary is packaged. Separate-process invocation is not a blanket legal conclusion.
- WebRTC in the local lab is provided by Electron's Chromium. No extra media library or remote ICE service is added.

Official references reviewed 2026-10-06:
- https://www.electronjs.org/docs/latest/tutorial/security
- https://github.com/rustdesk/rustdesk/blob/1.5.0/src/core_main.rs
- https://github.com/rustdesk/rustdesk/releases/tag/1.5.0
- https://github.com/rustdesk/rustdesk
- https://webrtc.org/getting-started/peer-connections
- https://webrtc.org/support/license
- https://tailscale.com/docs/reference/connection-types

## Measurement boundaries

The local lab sends a generated canvas through two real RTCPeerConnections with no ICE servers. It displays getStats frame dimensions, decoded FPS, codec and actual data-channel echo round trips. Input messages contain event kind and sequence only, no keys/text or clipboard content. Stop tears down tracks, peer connections and timers. Nothing controls the OS or records a real desktop.

A 3840x2160 requested source is not a 4K-delivery claim. Capture, encoded, decoded and display sizes differ; the lab reports actual decoded dimensions. It is not a benchmark of RustDesk, WAN latency, hardware encoding, real desktop capture or input-to-display latency. Do not use these results as proof of production remote-control speed.

## Next milestone gate

Use two specifically authorized owned computers with the reviewed engine. Verify peer identity, valid/invalid credentials, attended host approval, separate view/control/file refusal and revocation, actual mouse/keyboard behavior, disconnect/reconnect/cancel and held-input release. Start with measured 1080p and add QHD/4K, text/motion cases, direct/relay paths and CPU/GPU/bitrate/latency data.

An embedded professional client needs a supported session API or a separate reviewed native/WebRTC implementation. Resolve engine license/API strategy before that work. VPN/network enrollment and services are separate permission decisions. No unauthenticated legacy fallback is acceptable.
