# Connection wizard and Windows installer preview

This milestone is stacked on the reviewed Mac packaging HEAD
`19e056edb66c46f10045a00ce77bab0b722949b3`. It does not merge PR 67 or 68.

## User-visible changes

The Chinese home screen has two navigation destinations: computers and settings/help.
First use opens a four-step guide: existing connection program, local network facts,
peer ID/address annotation, and permission review. Back preserves a draft; cancel
discards it. Only the final save writes a device. A saved record does not launch
anything or preselect connection consent. Quick add remains available.

Unknown means unknown: adapter names/private IP ranges do not establish Tailscale
authentication, reachability, direct/relay routing or encryption. No Tailscale CLI
or remote probe is run. The external-session notice stays visible. The app cannot
disconnect RustDesk; after handoff the user must end that external session and
explicitly confirm before trying again. The synthetic lab is under advanced settings.

## Build

On an authorized Windows x64 build host with Node 22.12+:

```powershell
npm ci
npm test
npm run package:win -- $(git rev-parse HEAD)
```

Commit source changes first. Packaging stages only regular, allowlisted blobs
from that exact HEAD, reusing the Mac inventory checks. It never copies the
developer worktree. No production npm dependencies are supported without a staging
policy review. The verified payload includes runtime/project notices and a source
reference. A fresh installer project prevents implicit local build-resource hooks.

The output is `dist/RemoteControl-0.3.0-windows-x64-<HEAD12>-setup.exe`, plus a
SHA-256 file and JSON inventory. It is an actual NSIS installer, per-user only,
with a Start menu shortcut and uninstaller. Normal uninstall retains settings.
It does not create a desktop shortcut or automatically run the application after
installation; start RemoteControl from the Start menu. No service, VPN, firewall,
auto-start or unattended-access configuration is included. RustDesk is not bundled.

This preview is unsigned. No signing credentials are read. Respect Windows
security policy if it blocks the preview; do not disable protections. Authenticode
signing and acceptance on the user's device remain a separate delivery gate.

`tests/installer-smoke.cjs` runs only on a disposable GitHub-hosted Windows runner:
two real installs, all installed payload hashes, production IPC/security smoke,
and uninstall with a synthetic saved-settings canary. It must not run on the
shared user desktop. CI retains the installer and UI evidence as artifacts.

## Evidence and remaining boundaries

Browser review uses isolated headless Chrome, a loopback mock launcher and the
production validation/store model. Screenshots show real DOM rendering, not mockups.
Native Electron UI, packaged security and multi-instance tests run on Windows CI.
Wizard checks cover every cancel step, Escape, back/forward, invalid IDs/IPs,
double save, no premature persistence, and fresh consent. Additional browser
failure injection covers refresh/save retry and a 900px-wide layout.

Review found an inherited store defect: writes changed memory before the disk
save completed, so a failed/canceled edit could leak into later saves. The store
now serializes candidate writes and publishes an immutable snapshot only after
the temporary file is successfully renamed. Device changes, removal and engine
path selection share that transaction. A failed save leaves committed memory and
disk unchanged. Stable error codes select the Chinese ID/save messages; both ID
entry paths assert the complete translated message, including its punctuation.

`node tests/wizard-storage.cjs` runs a separate native Electron regression in CI:
an isolated profile contains a real filesystem obstruction, and the test checks
repeated failed saves, the full Chinese error, cancel, refresh, later successful
double-save and process restart. Only the later device may exist in memory/disk.
For headless local review set `REMOTECONTROL_STORAGE_BROWSER=1`; the loopback fixture
uses the same actual store/filesystem, without mocking the save response. Unit
tests also cover failed updates/removal/engine path, rename failure and concurrent
queued edits. The earlier HTTP failure test remains a transport-retry check; it
does not establish filesystem transaction behavior.

electron-builder 26.15.3 is pinned as a development dependency. npm audit on
2026-10-07 reported eight moderate entries along one transitive chain ending in
sprintf-js GHSA-hp3w-g68c-fv3c (unbounded format precision / denial of service).
That build-only dependency is excluded from ASAR. No forced dependency rewrite or
unsupported override was applied. It remains a build-tool limitation to monitor.

Mac packaging tooling is preserved, including the explicit allowlist update for
the new network module. This revision has not been natively built or visually
accepted on macOS. No remote-control engine was installed, paired, bundled or
connected by this milestone. Real session integration, authentication/encryption
acceptance and input-to-display performance remain incomplete.

## Next performance and integration milestone

Engine selection remains open. Compare native RustDesk as the complete remote
control baseline with Sunshine/Moonlight as the low-latency streaming baseline
after explicit installation/pairing approval and device authorization. Review
licensing before integrating or redistributing either engine; do not change the
repository license by implication.

Use the same external input-to-display measurement procedure for every engine,
with an approved test pattern and camera/timing apparatus. Report p50/p95 latency,
sample count and measurement uncertainty. Do not compare unlike overlay counters
as though they measure the same quantity. For each direction (Windows to Mac,
Mac to Windows), record host/client hardware and versions, 1080p60, QHD and 4K
requested/received resolution, actual presented/decoded FPS, dropped frames,
bitrate, codec, actual hardware encoder/decoder selection, CPU/GPU utilization,
and network conditions. Record Tailscale direct/relay/peer-relay state separately
from the engine's own transport. Obtain permission before network changes.

Connection acceptance also requires peer identity verification, rejected wrong
credentials, approval/denial/revocation, cancel during setup, disconnect/reconnect,
repeated actions, key release after interruption, and explicit failures without
insecure fallback. An encrypted VPN is not proof of application session security.
