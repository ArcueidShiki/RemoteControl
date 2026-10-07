# RemoteControl

The active client is now the cross-platform application in [desktop](desktop/README.md). The original MFC client/server source remains in RemoteCtrl for historical reference; it is not used by the new application.

## Current milestone
- Shared Windows/macOS interface with saved computers, local addresses and explicit connection consent.
- External RustDesk desktop/file-transfer handoff. RustDesk is obtained separately and owns authentication, encrypted sessions, host permissions, mouse/keyboard, media and disconnect.
- A local synthetic WebRTC lab reports actual decoded resolution/FPS and data-channel echo. It is not a production remote-desktop benchmark.
- Windows package build support; Mac source/build handoff, with actual Mac testing still required.
- Engine launch accepts only the reviewed Windows release digest; changed/unverified files and Mac engine launch are blocked pending identity review.

See [the architecture and limitations](desktop/docs/CURRENT-ARCHITECTURE.md), [Mac handoff](desktop/docs/MACOS-HANDOFF.md) and [legacy code audit](desktop/docs/LEGACY-AUDIT.md).

## Development
From desktop, run npm ci, npm test and npm start using Node 22.12 or newer. Windows installers require a clean committed checkout and an explicit HEAD: npm run package:win -- $(git rev-parse HEAD). Follow desktop/README.md for platform packaging and [the new connection wizard](desktop/docs/WIZARD-INSTALLER.md).

## Security
The original server has no authentication or transport encryption and must not be exposed publicly. The new client never falls back to it. Do not weaken Windows account, password, firewall or UAC policies to use this project. No VPN setup or node enrollment is performed by this milestone.

## Review workflow
Use a task branch, run the relevant tests, open a draft PR, complete independent review and required checks, then merge through the repository's normal protections. Do not force-push another person's work.

The repository license is unchanged. RustDesk is independently licensed and is not bundled or linked into the new client.
