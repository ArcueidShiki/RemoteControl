# macOS handoff

The same Electron source is intended for Windows/macOS. A Mac binary has not been built or tested. The owner's Mac is offline; no access has occurred. The current verifier deliberately blocks Mac engine launch until a reviewed executable digest or authentic publisher identity is added and tested there.

After explicit access authorization on the owner's Mac:
1. Install/use an approved Node version at least 22.12. Run npm ci from desktop.
2. Run npm test and npm run test:ui on that Mac. The test harness runs a hidden app with a fake engine launcher and real local WebRTC peers.
3. Run npm start, inspect layout, keyboard navigation and file picker. Review an independently obtained official RustDesk.app, validate its release provenance and signing identity, then add and test that identity in the engine verifier; do not bypass the gate.
4. Confirm the engine executable is resolved inside the application bundle and replaced/invalid applications are blocked. Run an actual session only with a specifically authorized owned target.
5. Verify host authentication and view/control/file permission denial/revocation, mouse/keyboard layouts and IME, Retina/DPI/multi-monitor behavior, focus loss, sleep/wake, repeated reconnect and disconnect in RustDesk.
6. Measure real 1080p first, then QHD/4K: actual received dimensions/FPS, input-to-display p50/p95, frame age, bitrate, hardware codec path and CPU/GPU/power. Separate direct, approved VPN and relay results.
7. Run npm run package:mac for Apple Silicon; use node scripts/package.cjs darwin x64 only if an Intel build is required and tested.

The packager refuses macOS builds on Windows. Signing, notarization and public distribution remain unconfigured; use only the owner's approved signing identity after a separate release review.

This shell neither captures the screen nor injects local OS input, so it does not request Screen Recording or Accessibility privileges. A RustDesk Mac host may require those permissions, which the owner must grant in the normal system UI. Do not bypass or modify TCC.

Apple references:
- https://support.apple.com/guide/mac-help/control-access-screen-system-audio-recording-mchld6aa7d23/mac
- https://support.apple.com/guide/mac-help/allow-accessibility-apps-to-access-your-mac-mh43185/mac
- https://developer.apple.com/documentation/screencapturekit
