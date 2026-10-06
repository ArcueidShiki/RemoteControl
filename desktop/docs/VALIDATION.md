# Milestone validation — 2026-10-06

## Passed locally
- Seven Node test suites: validation/injection, permission gate, cancellation, repeated handoffs, launch failure/stale callback recovery, metadata persistence, corrupt-settings preservation.
- 51 isolated browser-fixture UI checks using Chrome 154.0.8037.98 with Chromium sandboxing enabled. Engine process launch was mocked; device/session logic was the production model.
- Actual local WebRTC DTLS, decoded video and pointer/key-event data-channel echoes. No screen capture, clipboard, real remote peer or engine session was used.
- Windows x64 Electron package built successfully from the pinned runtime. The official runtime ZIP SHA-256 matched the npm package's pinned checksum.
- The separately downloaded official RustDesk 1.5.0 executable matched its GitHub release digest and had a valid Windows Authenticode signature. It was not installed, connected or bundled.

## Browser fixture measurements
Five approximately one-second decoded-FPS samples per case, after negotiation. Canvas requests 30 FPS. All cases negotiated VP8. These short measurements are diagnostic, not a statistically established performance claim.

| Requested source | Observed received size | Decoded FPS samples | Input echo samples (ms) |
| --- | --- | --- | --- |
| 1920x1080 | 1920x1080 | 30.2, 30.0, 29.7, 30.3, 30.0 | 2.1, 2.1, 3.7, 2.1, 2.4 |
| 2560x1440 | 2560x1440 | 30.0, 30.7, 29.3, 29.9, 30.7 | 3.3, 3.5, 2.3, 3.6, 2.2 |
| 3840x2160 | 3840x2160 | 24.0, 30.6, 24.2, 24.2, 29.7 | 2.2, 2.2, 2.2, 38.8, 47.3 |

Echo is a data-channel round trip on one computer, not remote input-to-display latency. These are not RustDesk, WAN, screen-capture, hardware-codec or production 4K results. Frame rate varied noticeably at 4K. Sample intervals can report slightly above 30 due to frame arrival timing. Re-run on target hardware and network conditions before setting product targets.

Reproduce the browser fallback: start node tests/browser-review.cjs, use the printed loopback URL as REMOTECONTROL_BROWSER_URL, then run node tests/electron-ui.cjs. Without that variable, the same harness runs the actual Electron app. Tests never launch a real remote engine.

## Remaining validation
The task folders inherit an AppContainer ACL that Electron 44 refuses for its sandboxed runtime. A copy in the permitted temporary directory hit the same restriction. Chromium sandboxing was not disabled and no ACL was changed. A narrow read/execute permission request is pending. Native Electron tests on this desktop therefore remain blocked; Windows CI is configured as a separate clean-runner validation path.

Real RustDesk authentication/encryption, host view/control approval, mouse/keyboard, file transfer, targeted disconnect and production performance still need two explicitly authorized owned endpoints. The external CLI cannot expose those outcomes to the shell.

macOS is source/build-handoff only; the owner's Mac is offline and not authorized. Signing/notarization and release publishing are not done. The PR remains draft pending native checks and independent review.
