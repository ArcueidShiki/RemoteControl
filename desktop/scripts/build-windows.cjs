'use strict';
// Compile a per-user installer from immutable, allowlisted Git blobs. Does not
// install it, launch an engine, use signing credentials, or publish artifacts.
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const assert = require('node:assert/strict');
const { execFileSync } = require('node:child_process');
const { stageFromGit, expectedAsarInventory, verifyAsar, treeInventory, sha256, gitBlob } = require('./macos-stage.cjs');
const root = path.resolve(__dirname, '..');
const repository = path.dirname(root);
let temporary;
const git = (...args) => execFileSync('git', args, { cwd: repository, encoding: 'utf8' }).trim();
async function checkedReplace(source, destination) {
  const resolved = path.resolve(destination), boundary = path.join(root, 'dist') + path.sep;
  if (!resolved.startsWith(boundary)) throw new Error('Output must remain inside desktop/dist.');
  await fs.rm(resolved, { recursive: true, force: true });
  await fs.cp(source, resolved, { recursive: true });
}
(async () => {
  const head = process.argv[2];
  if (process.platform !== 'win32' || process.arch !== 'x64') throw new Error('Build on an authorized native Windows x64 host.');
  if (process.argv.length !== 3 || !/^[a-f0-9]{40}$/.test(head || '')) throw new Error('Usage: node scripts/build-windows.cjs FULL_40_CHARACTER_HEAD');
  if (git('rev-parse', 'HEAD') !== head) throw new Error('HEAD does not match the selected build revision.');
  if (git('status', '--porcelain', '--untracked-files=normal')) throw new Error('Commit source changes before producing a revision-labelled installer.');
  temporary = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-windows-build-'));
  const stage = path.join(temporary, 'app');
  const staged = await stageFromGit(repository, head, stage);
  for (const [name, version] of Object.entries(staged.pkg.devDependencies)) {
    const installed = JSON.parse(await fs.readFile(path.join(root, 'node_modules', name, 'package.json'), 'utf8'));
    if (installed.version !== version) throw new Error('Run npm ci: version mismatch for ' + name);
  }
  const { packager } = await import('@electron/packager');
  const [output] = await packager({ dir: stage, out: path.join(temporary, 'packaged'), name: 'RemoteControl',
    platform: 'win32', arch: 'x64', electronVersion: staged.pkg.devDependencies.electron,
    electronZipDir: process.env.ELECTRON_ZIP_DIR || undefined, asar: true, prune: false,
    win32metadata: { ProductName: 'RemoteControl', FileDescription: 'RemoteControl connection preview' } });
  const asarInventory = await verifyAsar(path.join(output, 'resources', 'app.asar'), expectedAsarInventory(staged.files));
  for (const [name, source] of staged.documents) await fs.writeFile(path.join(output, name), source.data, { flag: 'wx' });
  await fs.writeFile(path.join(output, 'SOURCE-REFERENCE.json'), JSON.stringify({ sourceHead: head,
    sourceUrl: 'https://github.com/ArcueidShiki/RemoteControl/tree/' + head,
    electronSource: 'https://github.com/electron/electron/tree/v' + staged.pkg.devDependencies.electron,
    buildScriptSha256: sha256(gitBlob(repository, head, 'desktop/scripts/build-windows.cjs').data),
    lockfileSha256: staged.lockHash }, null, 2));
  await fs.writeFile(path.join(output, 'PREVIEW-BOUNDARIES.txt'),
    'Unsigned preview. No code-signing identity configured. Respect Windows security policy; do not bypass a block.\nExternal RustDesk is not bundled. No real remote session, encryption or performance acceptance.\nInstaller is per-user; no service, startup task, firewall or VPN change. Settings are retained on normal uninstall.\n');
  for (const notice of ['LICENSE', 'LICENSES.chromium.html', 'PROJECT-LICENSE.txt']) assert.ok((await fs.stat(path.join(output, notice))).size);
  // All build configuration is explicit. A pristine project directory prevents
  // auto-detection of local installer scripts, icons, hooks or certificates.
  const installerProject = path.join(temporary, 'installer-project'); await fs.mkdir(installerProject);
  await fs.writeFile(path.join(installerProject, 'package.json'), JSON.stringify({ name: staged.pkg.name, version: staged.pkg.version,
    description: staged.pkg.description, main: staged.pkg.main, author: 'ArcueidShiki' }));
  const outputBefore = await treeInventory(output);
  const { build, Platform, Arch } = require('electron-builder');
  const filename = 'RemoteControl-' + staged.pkg.version + '-windows-x64-' + head.slice(0, 12) + '-setup.exe';
  const installerOutput = path.join(temporary, 'installer-output');
  // Preview intentionally unsigned. Never discover or consume an OS signing key.
  process.env.CSC_IDENTITY_AUTO_DISCOVERY = 'false';
  const artifacts = await build({ projectDir: installerProject, prepackaged: output, publish: 'never',
    targets: Platform.WINDOWS.createTarget(['nsis'], Arch.x64),
    config: { appId: 'org.arcueidshiki.remotecontrol', productName: 'RemoteControl',
      directories: { output: installerOutput, buildResources: path.join(installerProject, 'no-resources') },
      electronVersion: staged.pkg.devDependencies.electron, npmRebuild: false, publish: null,
      win: { signAndEditExecutable: false, signExecutable: false, executableName: 'RemoteControl', requestedExecutionLevel: 'asInvoker' },
      nsis: { artifactName: filename, oneClick: true, perMachine: false, runAfterFinish: false,
        createDesktopShortcut: false, createStartMenuShortcut: true, shortcutName: 'RemoteControl',
        deleteAppDataOnUninstall: false, packElevateHelper: false, differentialPackage: false,
        installerLanguages: ['zh_CN', 'en_US'], license: path.join(output, 'PROJECT-LICENSE.txt') } } });
  // Installer generation must not silently modify the verified payload.
  assert.deepEqual(await treeInventory(output), outputBefore);
  const installer = path.join(installerOutput, filename);
  assert.ok(artifacts.some(file => path.resolve(file) === installer));
  const bytes = await fs.readFile(installer);
  assert.equal(bytes.subarray(0, 2).toString(), 'MZ', 'Expected a Windows PE installer.');
  const manifest = { schemaVersion: 1, sourceHead: head, version: staged.pkg.version, platform: 'win32', arch: 'x64',
    electron: staged.pkg.devDependencies.electron, builder: staged.pkg.devDependencies['electron-builder'],
    installer: { file: filename, bytes: bytes.length, sha256: sha256(bytes), signed: false, perMachine: false },
    lockfileSha256: staged.lockHash, asarInventory, packageInventory: outputBefore,
    validation: ['immutable Git allowlist', 'full ASAR inventory', 'installer build preserves verified payload'],
    notValidatedByBuild: ['installation/uninstallation', 'production GUI', 'remote session/authentication/encryption', 'remote input-to-display performance'] };
  const dist = path.join(root, 'dist'); await fs.mkdir(dist, { recursive: true });
  await checkedReplace(output, path.join(dist, 'RemoteControl-win32-x64'));
  await fs.copyFile(installer, path.join(dist, filename));
  await fs.writeFile(path.join(dist, filename + '.sha256'), manifest.installer.sha256 + '  ' + filename + '\n');
  await fs.writeFile(path.join(dist, filename + '.json'), JSON.stringify(manifest, null, 2) + '\n');
  console.log(JSON.stringify({ sourceHead: head, installer: manifest.installer, asarFiles: Object.keys(asarInventory) }, null, 2));
})().catch(error => { console.error(error); process.exitCode = 1; })
  .finally(async () => {
    if (temporary && path.resolve(temporary).startsWith(path.resolve(os.tmpdir()) + path.sep + 'remotecontrol-windows-build-'))
      await fs.rm(temporary, { recursive: true, force: true });
  });
