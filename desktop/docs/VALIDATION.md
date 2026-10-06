# Milestone validation — 2026-10-06

## Passed locally
- Thirteen Node tests: validation/injection, permission gate, cancellation including asynchronous verification, repeated handoffs, launch failure/stale callback recovery, metadata persistence, corrupt-settings preservation, saved/renamed impostor rejection, content digest rejection, OS environment allowlists and actual child-process environment/cwd isolation.
- 51 isolated browser-fixture UI checks using Chrome 154.0.8037.98 with Chromium sandboxing enabled. Engine process launch was mocked; device/session logic was the production model.
- Actual local WebRTC DTLS, decoded video and pointer/key-event data-channel echoes. No screen capture, clipboard, real remote peer or engine session was used.
- Windows x64 Electron package built successfully from the pinned runtime. The official runtime ZIP SHA-256 matched the npm package's pinned checksum.
- The separately downloaded official RustDesk 1.5.0 executable matched its GitHub release digest and had a valid Windows Authenticode signature during the initial online review. Runtime verification pins SHA-256; it does not rely on online signature checks. The integration verifier accepted the real file and restored saved path, captured repeated handoff arguments without executing them, and rejected a same-size mutation made after selection/prepare. The engine was not installed, connected or bundled on this desktop.

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
The task folders inherit an AppContainer ACL that Electron 44 refuses for its sandboxed runtime. A copy in the permitted temporary directory hit the same restriction. Chromium sandboxing was not disabled and no ACL was changed. Native Electron tests on this desktop remain blocked; clean Windows CI is the validation path. The previous head c8002bc passed 52 native Electron UI checks and packaging. The hardening revision adds a packaged ASAR startup/real-IPC test with production verification, a saved impostor and the development mock flag deliberately present. It also adds a real trusted RustDesk --version invocation in a disposable Windows CI profile using the production launch environment/cwd helper. Exact-head CI results are recorded in the PR/review evidence, separately from this test description.

The version probe never receives connection, credential or installation arguments. It is restricted to disposable hosted Windows runners because RustDesk's portable wrapper extracts into its Windows profile even for --version. No RustDesk process is started on the shared desktop. The local trusted-engine test is verification-only. The probe proves executable startup/CLI compatibility, not desktop/file handoff completion or a two-machine remote session.

Real RustDesk authentication/encryption, host view/control approval, mouse/keyboard, file transfer, targeted disconnect and production performance still need two explicitly authorized owned endpoints. The external CLI cannot expose those outcomes to the shell.

macOS is source/build-handoff only; the owner's Mac is offline and has not been accessed. Mac engine launch remains blocked until its executable identity is reviewed and added to the verifier. Signing/notarization and release publishing are not done. The PR remains draft pending exact-head checks and independent rereview.
