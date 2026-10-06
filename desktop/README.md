# RemoteControl desktop — milestone 0.2

A shared Windows/macOS workspace with an external RustDesk engine. This replaces the MFC application as the active direction; it does not wrap or launch the legacy server.

## Run
Use Node 22.12+:
- npm ci
- npm start
- npm test
- npm run test:ui

In the app, choose the official RustDesk executable in Settings, add a computer's RustDesk ID, confirm permission and continue. Desktop and file-transfer actions open RustDesk's own window. Complete authentication, verify the peer, choose view-only/control and disconnect there. A peer IP entered here is a label, not verified session metadata.

RustDesk is not bundled or silently installed. Obtain it separately from https://github.com/rustdesk/rustdesk/releases and verify the publisher/signature. This adapter was inspected against the official 1.5.0 command parser. It passes only --connect ID or --file-transfer ID, never passwords, configuration changes, installation/elevation flags or shell text. It does not contact the legacy server.

## Honest boundaries
- This is a working external-engine integration milestone, not an embedded remote-desktop implementation or a complete professional product.
- The shell cannot enforce view-only, inspect host approval, observe authentication/encryption status, read the session resolution/FPS, or disconnect an existing RustDesk session. It labels process launch as handoff, not “connected.” Do not assume closing the shell ends a session.
- RustDesk owns mouse/keyboard, media codecs, file permissions and remote session behavior. Its configured rendezvous/relay service is used unchanged. An encrypted session and target performance have NOT been validated end-to-end on an authorized second computer during this task.
- Direct-IP launch is deliberately absent: an address alone is not peer identity or proof of encryption.
- No LAN scan, automatic discovery, VPN enrollment, credential creation, firewall/startup change, server deployment or recording is included.
- Saved metadata: device names, numeric IDs, optional IP labels, selected executable. No passwords or clipboard contents.
- The local WebRTC lab uses a generated canvas and two peers on this machine with no ICE servers. It measures actual decoded dimensions/FPS and data-channel echo. It does not capture the screen or control the OS, and its results are not RustDesk/WAN/input-to-display benchmarks.

## Build and package
- Windows x64: npm run package:win
- On an authorized Apple Silicon Mac: npm ci; npm test; npm run test:ui; npm run package:mac
- Intel Mac, if needed: node scripts/package.cjs darwin x64

The packager refuses a Mac build on Windows. Signing/notarization are not configured. Do not claim a tested Mac package until these commands and real UI/permission/session tests pass on the owner's authorized Mac.

ELECTRON_ZIP_DIR may point to a directory containing the pinned official Electron ZIP to avoid another download. The source lockfile pins dependencies. REMOTECONTROL_PROFILE sets a separate local profile for testing. REMOTECONTROL_TEST only works in unpackaged development; it substitutes the engine launcher, never the WebRTC lab. Tests run an invisible Electron window and never control shared desktop windows.

## Licensing and security
The repository license remains unchanged (GPL v2 file at ../LICENSE). Electron is MIT licensed and its runtime notices ship with the Windows package. RustDesk is independently AGPL-3.0 licensed; no RustDesk source is copied/linked and its executable is excluded from the package. Separate-process invocation is an integration boundary, not a legal conclusion about every future distribution. Review licensing before bundling, modifying or embedding an engine.

Renderer: sandboxed, context isolation, Node integration off, restrictive CSP, packaged local assets only. IPC validates the sender and exposes specific methods rather than arbitrary shell/file APIs. Engine launch uses spawn with shell:false and fixed argument arrays. Permission defaults off and resets per attempt. No remote fonts, analytics or auto-updater.

## Next acceptance stage
Use two explicitly authorized owned machines to validate the actual RustDesk path: verify identities, reject invalid credentials, deny/revoke view/control, test mouse/keyboard and file transfer, close/disconnect/reconnect, and measure actual 1080p/QHD/4K performance on direct and approved relay/VPN routes. Deeper integration requires a supported engine session API or a reviewed native/WebRTC implementation. The current CLI cannot truthfully provide embedded session controls.
