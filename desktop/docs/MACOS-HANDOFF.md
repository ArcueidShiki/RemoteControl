# macOS build handoff

## Scope and baseline

This document preserves the historical Mac build evidence and current Mac safety
gates. Active build/debug/test instructions are in the source repository's root
BUILD.md. In a packaged copy, use SOURCE-REFERENCE.json's sourceUrl to find it.
The later cleanup removes retired MFC projects, not this handoff or the shared
macos-stage.cjs module used by both platforms. Historical Mac results below do
not establish native acceptance of the current shared HEAD.

The authorized Mac was inspected on 2026-10-07: Apple Silicon arm64,
macOS 26.4.1, Node 23.11.0 / npm 10.9.2, pnpm 10.15.0, Rust 1.90.0,
Xcode 27.0 beta (27A5228h), macOS SDK 27.0. No existing RemoteControl or
RustDesk app was found in /Applications or ~/Applications. No system toolchain
installation or switch was needed. Node 23 is the available local toolchain;
use the repository's Node 22.12+ requirement and pin the chosen version in CI.
The MFC/Win32 legacy programs cannot be built as Mac applications here.

This milestone starts from PR #67 at
`d76efde588016cfd70216b2ac7d1170b3415ef3f`. It adds Mac build tooling and a
physical-path correction to the child-process isolation test. It does not
change shared UI, Windows packaging, the engine allowlist or network setup.
The current product remains an Electron workspace with an external launcher,
not a complete remote-control application. Mac engine launch is blocked.

## Repeatable preview build

From a clean checkout of the exact revision selected for review:

```sh
cd desktop
npm ci
node scripts/build-macos.cjs FULL_40_CHARACTER_COMMIT_SHA arm64
```

The script requires macOS, a matching native architecture, the specified HEAD,
a clean repository and installed direct dependencies matching package.json.
Git status alone is not the packaging boundary: ignored files can exist in a
clean checkout. The script reads only explicit regular-file Git blobs from the
selected commit into a fresh temporary staging directory. It never copies the
developer checkout or its node_modules into the application. App files and
distribution documents have separate allowlists in scripts/macos-stage.cjs.
The runtime package.json is deterministically reduced to name, version,
description and main. Source file bytes otherwise come directly from Git.

The current app has no npm production dependencies. Its lockfile must agree
with package.json and contain no production package entries. Adding a runtime
dependency fails closed until a reviewed production-dependency allowlist and
clean locked install are implemented; dependencies are never silently dropped.
Electron remains the pinned external runtime and its notices are retained.

After the Node tests, Packager receives only the clean stage and a new output
directory. The build checks bundle ID/version, Mach-O architecture and the
complete ASAR inventory (every path/type/content hash/executable flag). It then
creates a ZIP without resource forks or extended attributes, checks every ZIP
entry, extracts it and compares every file hash, directory, executable flag and
symlink target against the clean package. Links escaping the bundle root fail.
Only successful results replace the generated app directory under dist.
The legacy package:mac shortcut now refuses the broad-copy path.
Do not treat this as bit-for-bit reproducibility: filesystem timestamps,
toolchain and generated metadata may differ between runs.

Outputs under `desktop/dist/`:

- `RemoteControl-darwin-arm64/RemoteControl.app`
- `RemoteControl-VERSION-macos-arm64-SHA-unsigned-preview.zip`
- The matching `.json` build manifest and `.zip.sha256` checksum

The manifest records the source HEAD, Node/macOS/Electron versions, lockfile
hash, complete ASAR/package inventories, declared minimum OS, signing inspection,
checks and unvalidated scope. The wrapper replaces an existing ZIP for the same
revision. Old pre-fix preview archives are superseded and are not release inputs;
the ignored-file review finding was a packaging risk, not evidence of a leak.
For Intel, run on an authorized Intel Mac with `x64`. Cross-architecture and
universal builds are not claimed tested. No DMG/PKG or final installer is
produced by this milestone; wait for the agreed UI/architecture HEAD first.

`npm ci` uses the pinned lockfile and official npm/Electron distribution.
Electron may download its runtime lazily on first require/packaging; a successful
npm install alone does not prove the runtime is present. The standard Electron
downloader checks release checksums. `ELECTRON_ZIP_DIR` is supported by the
existing packager but should only point to independently verified official
runtime assets. No RustDesk executable is downloaded or bundled by this build.

## Verification and current limits

On this Mac, the original 14 Node tests passed after the physical-path fix.
The security revision adds four regression tests using only synthetic fixture
repositories: ignored .pfx canaries, ignored node_modules/output, untracked
build/source files, modified worktree source/docs, extra/altered/linked ASAR
entries, production-dependency rejection, duplicate ZIP entries and escaping
bundle links. The Mac fixture also creates and extracts a real ZIP.
A native
arm64 app and roughly 129 MB preview ZIP were built successfully, with declared
minimum macOS 13.0. Bundle metadata, architecture, ASAR/source equality and ZIP
integrity passed. `codesign -d` reports ad-hoc/linker signing, no TeamIdentifier,
unbound Info.plist and no sealed resources; this is not publisher signing.

The hidden development UI test was attempted but Electron failed before its
debugger handshake. A diagnostic attempt reported `kill EPERM`; read-only
`codesign --verify --deep --strict` on the downloaded development runtime
reported “code has no resources but signature indicates they must be present”.
`spctl --assess` returned an internal Code Signing subsystem error in this
execution environment. This does not establish a single root cause or a
Gatekeeper verdict. No successful native UI test is claimed, and no quarantine,
manual re-signing, sandbox or Gatekeeper workaround was applied. Resolve through a normal
verified runtime/signing path before attempting production GUI acceptance.

`npm test` checks validation, consent/cancellation, saved-path and content-digest
rejection, environment isolation and the single-instance startup gate.
macOS reports a physical `/private/var/...` cwd for temporary paths created via
`/var/...`; the isolation assertion now compares `fs.realpath(directory)`.
This preserves the same assertion about directory identity and secret removal.

On the authorized Mac, `npm run test:ui` can run the existing hidden development
window with fake engine launches and synthetic local WebRTC peers. It does not
capture the screen or inject OS input. Results must be recorded separately;
synthetic canvas decode/FPS is not remote desktop or WAN performance.
The existing `test:packaged` and `test:instances` commands hard-code the Windows
EXE. Do not claim them passed on Mac; adapt these with the shared UI owner once
the common HEAD is agreed. The build manifest intentionally claims only static
bundle checks and Node tests, not production GUI/session validation.

No Developer ID signing or notarization is configured. Packager 20.3.0 normally
patches the Electron framework integrity digest and automatically ad-hoc signs
that framework. These operations and upstream/ad-hoc signatures do not establish
this app's publisher identity or Gatekeeper approval. No manual re-signing was
used to force a failed launch. Do not remove quarantine, disable Gatekeeper,
bypass warnings, ad-hoc re-sign as a workaround, or silently install.
A distributable installer needs the approved signing/notarization path and
an independent review of the final revision.

The build checks that Electron's LICENSE and LICENSES.chromium.html plus the
repository's PROJECT-LICENSE.txt exist and are nonempty; all are included in
the verified ZIP inventory. SOURCE-REFERENCE.json identifies the exact public
repository commit, relevant build scripts/lockfile/test hashes and the matching
Electron source tag. This records available source/build references and notices;
it is not a complete dependency-license compliance audit, nor approval for
integrating or relicensing any engine.

## Mac engine acceptance gate

Use only the [official RustDesk 1.5.0 release](https://github.com/rustdesk/rustdesk/releases/tag/1.5.0)
as the candidate source. An official download link alone does not validate a
local bundle. Before enabling Mac launch, record the release asset hash and
architecture, inspect its bundle identifier and executable, validate the full
bundle/signature and approved publisher identity, and check normal Gatekeeper
assessment. Review embedded libraries/resources as well as the main executable;
a main-binary hash alone does not authenticate an entire .app. Test replacement,
mutation, symlink resolution, saved-path restoration and launch-time rechecking.
Do not guess a signing Team ID or silently allow unsigned/unreviewed assets.

The current picker assumes `Contents/MacOS/RustDesk`; confirm the actual
`CFBundleExecutable`, case and architecture from the reviewed upstream bundle
before changing it. Unsupported paths/releases must continue to fail closed.
The Windows minimal environment does not establish Mac engine compatibility;
validate the Mac launch environment without forwarding credentials or loader
variables. Do not add credentials, unattended access or installation flags.

## Coordinated final acceptance and installation

Agree one common HEAD with the Windows/UI owner before producing final
Windows/macOS installers. Review the first-run guide and native app lifecycle,
then test packaged production IPC, ignored development flags and competing
instances with an isolated profile on a private desktop. Do not replace this
with a development mock-only result.

For a real remote session, identify both owned endpoints and an attended host
operator first. Keep existing Tailscale/VPN, firewall and listener configuration
unchanged. A RustDesk ID handoff does not prove that traffic used Tailscale;
verify the actual route and authenticated peer before reporting it as tested.
Test incorrect credentials, host denial/revocation, view-only enforcement,
keyboard/mouse, IME/Retina/multi-monitor behavior, reconnect and actual disconnect.
Measure real received resolution/FPS, latency percentiles, frame age and codec
path at 1080p before QHD/4K. Record direct/VPN/relay results separately.

This shell needs neither Screen Recording nor Accessibility access. If a
reviewed RustDesk Mac host later needs Screen Recording for outgoing screen
frames or Accessibility for incoming keyboard/mouse, state that exact purpose
and ask the owner to grant it through normal System Settings. No TCC change,
camera/microphone use, personal-data access or persistent unattended password
is authorized here.

After final review, report the exact source artifact, destination app path,
existing-app replacement behavior and expected profile writes before executing
the already-requested install. Stop for any new macOS security confirmation.
This preview stage does not copy to /Applications or start a remote engine.

References: [Electron code signing](https://www.electronjs.org/docs/latest/tutorial/code-signing),
[Apple screen recording access](https://support.apple.com/guide/mac-help/control-access-screen-system-audio-recording-mchld6aa7d23/mac),
[Apple Accessibility access](https://support.apple.com/guide/mac-help/allow-accessibility-apps-to-access-your-mac-mh43185/mac).
