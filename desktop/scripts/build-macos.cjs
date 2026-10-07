'use strict';
// Local preview packaging only. No installation, engine launch, signing identity,
// notarization, permissions, network configuration or security-policy changes.
const fs = require('node:fs/promises');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { spawnSync } = require('node:child_process');
const root = path.resolve(__dirname, '..');
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
  const pkg = require('../package.json');
  for (const [name, version] of Object.entries(pkg.devDependencies)) {
    const installed = JSON.parse(await fs.readFile(path.join(root, 'node_modules', name, 'package.json'), 'utf8'));
    if (installed.version !== version) throw new Error(`Run npm ci: installed ${name} differs from the pinned version.`);
  }
  run(process.execPath, ['--test', ...((await fs.readdir(path.join(root, 'tests')))
    .filter(name => name.endsWith('.test.cjs')).sort().map(name => path.join('tests', name)))]);
  run(process.execPath, ['scripts/package.cjs', 'darwin', arch]);
  const output = path.join(root, 'dist', `RemoteControl-darwin-${arch}`);
  const app = path.join(output, 'RemoteControl.app');
  const plist = path.join(app, 'Contents', 'Info.plist');
  const info = JSON.parse(run('/usr/bin/plutil', ['-convert', 'json', '-o', '-', plist], true));
  if (info.CFBundleIdentifier !== 'org.arcueidshiki.remotecontrol' || info.CFBundleShortVersionString !== pkg.version)
    throw new Error('Packaged bundle metadata does not match source.');
  const executable = path.join(app, 'Contents', 'MacOS', info.CFBundleExecutable);
  const binaryArch = run('/usr/bin/lipo', ['-archs', executable], true);
  if (binaryArch !== (arch === 'x64' ? 'x86_64' : 'arm64')) throw new Error('Unexpected Mach-O architecture.');
  const asar = await import('@electron/asar');
  const archive = path.join(app, 'Contents', 'Resources', 'app.asar');
  for (const name of await fs.readdir(path.join(root, 'src'))) {
    if (!(await fs.stat(path.join(root, 'src', name))).isFile()) throw new Error('Review new source directories before packaging.');
    if (!asar.extractFile(archive, `src/${name}`).equals(await fs.readFile(path.join(root, 'src', name))))
      throw new Error(`ASAR differs from source: ${name}`);
  }
  const entries = asar.listPackage(archive);
  if (entries.some(name => /[\\/](tests|scripts|node_modules|\.userdata|dist)([\\/]|$)/.test(name)))
    throw new Error('Development files leaked into ASAR.');
  const boundary = 'LOCAL LAUNCHER PREVIEW ONLY\nNo Developer ID signing or notarization. macOS may block opening it; do not bypass Gatekeeper.\nRustDesk is not bundled. Mac engine launch remains blocked pending independent identity review.\nNo real remote authentication, desktop, keyboard/mouse, file transfer or WAN performance has been validated.\n';
  await fs.writeFile(path.join(output, 'PREVIEW-BOUNDARIES.txt'), boundary);
  await fs.copyFile(path.join(root, 'docs', 'MACOS-HANDOFF.md'), path.join(output, 'MACOS-HANDOFF.md'));
  const name = `RemoteControl-${pkg.version}-macos-${arch}-${head.slice(0, 12)}-unsigned-preview`;
  const zip = path.join(root, 'dist', name + '.zip');
  await fs.rm(zip, { force: true });
  run('/usr/bin/ditto', ['-c', '-k', '--sequesterRsrc', '--keepParent', output, zip]);
  run('/usr/bin/unzip', ['-tq', zip]);
  const signature = spawnSync('/usr/bin/codesign', ['-d', '--verbose=4', app], { encoding: 'utf8' });
  const manifest = { schemaVersion: 1, builtAt: new Date().toISOString(), sourceHead: head,
    version: pkg.version, platform: 'darwin', arch, node: process.version,
    macOS: run('/usr/bin/sw_vers', ['-productVersion'], true), electron: pkg.devDependencies.electron,
    minimumSystemVersion: info.LSMinimumSystemVersion || null,
    lockfileSha256: await digest(path.join(root, 'package-lock.json')),
    signing: 'No Developer ID signing; upstream/ad-hoc signatures may exist. Not notarized.',
    codesignDisplayExitCode: signature.status, codesignDisplay: signature.stderr.trim(),
    validation: ['Node tests passed', 'bundle metadata and native Mach-O architecture checked',
      'ASAR source bytes matched', 'development directories excluded from ASAR', 'ZIP integrity checked'],
    notValidated: ['production GUI', 'Gatekeeper acceptance', 'remote engine/session/authentication/input', 'remote FPS/WAN'],
    artifact: { file: path.basename(zip), bytes: (await fs.stat(zip)).size, sha256: await digest(zip) } };
  await fs.writeFile(path.join(root, 'dist', name + '.json'), JSON.stringify(manifest, null, 2) + '\n');
  await fs.writeFile(zip + '.sha256', `${manifest.artifact.sha256}  ${path.basename(zip)}\n`);
  console.log(JSON.stringify(manifest, null, 2));
})().catch(error => { console.error(error.message); process.exitCode = 1; });
