'use strict';
// Local preview packaging only. No installation, engine launch, signing identity,
// notarization, permissions, network configuration or security-policy changes.
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const assert = require('node:assert/strict');
const { createHash } = require('node:crypto');
const { spawnSync } = require('node:child_process');
const root = path.resolve(__dirname, '..');
const { stageFromGit, expectedAsarInventory, verifyAsar, treeInventory, verifyZipNames, gitBlob, sha256 } = require('./macos-stage.cjs');
let temporary;
function run(command, args, capture = false) {
  const result = spawnSync(command, args, { cwd: root, encoding: 'utf8', shell: false,
    stdio: capture ? 'pipe' : 'inherit' });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(`${command} failed (${result.status}): ${result.stderr || ''}`);
  return (result.stdout || '').trim();
}
async function digest(file) {
  const hash = createHash('sha256');
  const handle = await fs.open(file, 'r');
  try { for await (const chunk of handle.createReadStream()) hash.update(chunk); }
  finally { await handle.close(); }
  return hash.digest('hex');
}
(async () => {
  const args = process.argv.slice(2);
  if (args.length !== 2 || !/^[a-f0-9]{40}$/.test(args[0]) || !['arm64', 'x64'].includes(args[1]))
    throw new Error('Usage: node scripts/build-macos.cjs EXPECTED_40_CHARACTER_HEAD arm64|x64');
  if (process.platform !== 'darwin') throw new Error('Build on the authorized Mac.');
  const [expectedHead, arch] = args;
  if (arch !== process.arch) throw new Error('Preview validation requires a native host of the requested architecture.');
  const head = run('git', ['rev-parse', 'HEAD'], true);
  if (head !== expectedHead) throw new Error('HEAD differs from the explicitly selected build revision.');
  if (run('git', ['status', '--porcelain', '--untracked-files=normal'], true))
    throw new Error('Commit or remove source changes before producing revision-labelled artifacts.');
  temporary = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-macos-build-'));
  const stage = path.join(temporary, 'app');
  const staged = await stageFromGit(path.dirname(root), head, stage);
  const { pkg } = staged;
  for (const [name, version] of Object.entries(pkg.devDependencies)) {
    const installed = JSON.parse(await fs.readFile(path.join(root, 'node_modules', name, 'package.json'), 'utf8'));
    if (installed.version !== version) throw new Error(`Run npm ci: installed ${name} differs from the pinned version.`);
  }
  run(process.execPath, ['--test', ...((await fs.readdir(path.join(root, 'tests')))
    .filter(name => name.endsWith('.test.cjs')).sort().map(name => path.join('tests', name)))]);
  const { packager } = await import('@electron/packager');
  const outputs = await packager({ dir: stage, out: path.join(temporary, 'packaged'), name: 'RemoteControl', platform: 'darwin', arch,
    electronVersion: pkg.devDependencies.electron, electronZipDir: process.env.ELECTRON_ZIP_DIR || undefined,
    asar: true, prune: false, appBundleId: 'org.arcueidshiki.remotecontrol', appCategoryType: 'public.app-category.productivity' });
  assert.equal(outputs.length, 1);
  const output = outputs[0];
  const app = path.join(output, 'RemoteControl.app');
  const plist = path.join(app, 'Contents', 'Info.plist');
  const info = JSON.parse(run('/usr/bin/plutil', ['-convert', 'json', '-o', '-', plist], true));
  if (info.CFBundleIdentifier !== 'org.arcueidshiki.remotecontrol' || info.CFBundleShortVersionString !== pkg.version)
    throw new Error('Packaged bundle metadata does not match source.');
  const executable = path.join(app, 'Contents', 'MacOS', info.CFBundleExecutable);
  const binaryArch = run('/usr/bin/lipo', ['-archs', executable], true);
  if (binaryArch !== (arch === 'x64' ? 'x86_64' : 'arm64')) throw new Error('Unexpected Mach-O architecture.');
  const archive = path.join(app, 'Contents', 'Resources', 'app.asar');
  const asarInventory = await verifyAsar(archive, expectedAsarInventory(staged.files));
  for (const [name, source] of staged.documents) await fs.writeFile(path.join(output, name), source.data, { flag: 'wx' });
  const boundary = 'LOCAL LAUNCHER PREVIEW ONLY\nNo Developer ID signing or notarization. macOS may block opening it; do not bypass Gatekeeper.\nRustDesk is not bundled. Mac engine launch remains blocked pending independent identity review.\nNo real remote authentication, desktop, keyboard/mouse, file transfer or WAN performance has been validated.\n';
  await fs.writeFile(path.join(output, 'PREVIEW-BOUNDARIES.txt'), boundary);
  const buildSources = ['desktop/scripts/build-macos.cjs', 'desktop/scripts/macos-stage.cjs',
    'desktop/package.json', 'desktop/package-lock.json', 'desktop/tests/macos-stage.test.cjs'];
  const reference = { sourceHead: head, sourceUrl: `https://github.com/ArcueidShiki/RemoteControl/tree/${head}`,
    buildInputs: Object.fromEntries(buildSources.map(name => { const source = gitBlob(path.dirname(root), head, name);
      return [name, { gitBlob: source.blob, sha256: sha256(source.data) }]; })),
    runtime: { name: 'Electron', version: pkg.devDependencies.electron,
      sourceUrl: `https://github.com/electron/electron/tree/v${pkg.devDependencies.electron}` },
    note: 'Source/build references and shipped notices are recorded; this is not a complete dependency-license compliance audit.' };
  await fs.writeFile(path.join(output, 'SOURCE-REFERENCE.json'), JSON.stringify(reference, null, 2) + '\n', { flag: 'wx' });
  for (const notice of ['LICENSE', 'LICENSES.chromium.html', 'PROJECT-LICENSE.txt'])
    assert.ok((await fs.stat(path.join(output, notice))).size > 0, `Required runtime/project notice is missing: ${notice}`);
  const packageInventory = await treeInventory(output);
  const name = `RemoteControl-${pkg.version}-macos-${arch}-${head.slice(0, 12)}-unsigned-preview`;
  const zip = path.join(temporary, name + '.zip');
  // Do not import resource forks, Finder metadata or extended attributes into ZIP.
  run('/usr/bin/ditto', ['-c', '-k', '--norsrc', '--noextattr', '--noqtn', '--keepParent', output, zip]);
  run('/usr/bin/unzip', ['-tq', zip]);
  verifyZipNames(run('/usr/bin/unzip', ['-Z1', zip], true).split('\n'), path.basename(output), packageInventory);
  const unpacked = path.join(temporary, 'unpacked');
  await fs.mkdir(unpacked);
  run('/usr/bin/ditto', ['-x', '-k', '--norsrc', '--noextattr', '--noqtn', zip, unpacked]);
  assert.deepEqual(await fs.readdir(unpacked), [path.basename(output)]);
  assert.deepEqual(await treeInventory(path.join(unpacked, path.basename(output))), packageInventory,
    'Extracted ZIP file hashes, executable flags, directories and symlink targets differ.');
  const signature = spawnSync('/usr/bin/codesign', ['-d', '--verbose=4', app], { encoding: 'utf8' });
  const manifest = { schemaVersion: 2, builtAt: new Date().toISOString(), sourceHead: head,
    version: pkg.version, platform: 'darwin', arch, node: process.version,
    macOS: run('/usr/bin/sw_vers', ['-productVersion'], true), electron: pkg.devDependencies.electron,
    minimumSystemVersion: info.LSMinimumSystemVersion || null,
    lockfileSha256: staged.lockHash,
    signing: 'No Developer ID signing or notarization. Packager may automatically patch and ad-hoc sign its framework; no manual re-signing workaround.',
    codesignDisplayExitCode: signature.status, codesignDisplay: signature.stderr.trim(),
    validation: ['Node tests passed', 'bundle metadata and native Mach-O architecture checked',
      'complete ASAR inventory matched immutable Git allowlist', 'ZIP names and extracted inventory matched the clean package',
      'runtime/project notices and source/build references present'],
    asarInventory, packageInventory,
    notValidated: ['production GUI', 'Gatekeeper acceptance', 'remote engine/session/authentication/input', 'remote FPS/WAN'],
    artifact: { file: path.basename(zip), bytes: (await fs.stat(zip)).size, sha256: await digest(zip) } };
  const dist = path.join(root, 'dist'); await fs.mkdir(dist, { recursive: true });
  // Publish only after all inventory checks pass. Never overlay stale app files.
  const appOutput = path.join(dist, path.basename(output));
  await fs.rm(appOutput, { recursive: true, force: true });
  await fs.cp(output, appOutput, { recursive: true, verbatimSymlinks: true });
  await fs.copyFile(zip, path.join(dist, path.basename(zip)));
  await fs.writeFile(path.join(dist, name + '.json'), JSON.stringify(manifest, null, 2) + '\n');
  await fs.writeFile(path.join(dist, path.basename(zip) + '.sha256'), `${manifest.artifact.sha256}  ${path.basename(zip)}\n`);
  console.log(JSON.stringify({ ...manifest, asarInventory: Object.keys(asarInventory), packageInventory: `${Object.keys(packageInventory).length} entries recorded in manifest` }, null, 2));
})().catch(error => { console.error(error.message); process.exitCode = 1; })
  .finally(async () => { if (temporary) await fs.rm(temporary, { recursive: true, force: true }); });
