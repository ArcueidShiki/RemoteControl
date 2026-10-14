'use strict';
// A real per-user install modifies HKCU and the Start menu. This regression is
// deliberately restricted to disposable GitHub-hosted Windows workers.
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { treeInventory } = require('../scripts/macos-stage.cjs');
const root = path.resolve(__dirname, '..');
const run = (command, args, env = process.env) => new Promise((resolve, reject) => {
  const child = spawn(command, args, { cwd: root, shell: false, windowsHide: true, env, stdio: 'inherit' });
  const timer = setTimeout(() => { child.kill(); reject(new Error('Installer regression timed out.')); }, 120000);
  child.on('error', error => { clearTimeout(timer); reject(error); });
  child.on('exit', code => { clearTimeout(timer); code === 0 ? resolve() : reject(new Error('Process failed: ' + code)); });
});
let temporary, profile;
(async () => {
  if (process.platform !== 'win32' || process.env.GITHUB_ACTIONS !== 'true' || process.env.RUNNER_ENVIRONMENT !== 'github-hosted')
    throw new Error('Install/uninstall regression requires a disposable GitHub-hosted Windows runner.');
  const revision = require('node:child_process').execFileSync('git', ['rev-parse', 'HEAD'], { cwd: root, encoding: 'utf8' }).trim();
  const manifestName = (await fs.readdir(path.join(root, 'dist'))).find(name => name.endsWith('-setup.exe.json') && name.includes(revision.slice(0, 12)));
  assert.ok(manifestName, 'No exact-head installer manifest.');
  const manifest = JSON.parse(await fs.readFile(path.join(root, 'dist', manifestName), 'utf8'));
  assert.equal(manifest.sourceHead, revision);
  const installer = path.join(root, 'dist', manifest.installer.file);
  const { createHash } = require('node:crypto');
  assert.equal(createHash('sha256').update(await fs.readFile(installer)).digest('hex'), manifest.installer.sha256);
  temporary = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-installer-test-'));
  const installed = path.join(temporary, 'installed');
  // Own only a newly created, otherwise absent application profile.
  const expectedProfile = path.join(process.env.APPDATA, 'remotecontrol-desktop');
  await fs.mkdir(expectedProfile); profile = expectedProfile;
  const saved = '{"devices":[],"enginePath":"","testCanary":"preserve through uninstall"}';
  await fs.writeFile(path.join(profile, 'devices.json'), saved);
  for (let iteration = 0; iteration < 2; iteration++) {
    await run(installer, ['/S', '/D=' + installed]);
    const inventory = await treeInventory(installed);
    for (const [name, expected] of Object.entries(manifest.packageInventory)) assert.deepEqual(inventory[name], expected, 'Installed payload differs: ' + name);
    const extra = Object.keys(inventory).filter(name => !Object.hasOwn(manifest.packageInventory, name));
    assert.deepEqual(extra, ['Uninstall RemoteControl.exe'], 'Unexpected installer payload additions.');
    assert.equal(await fs.readFile(path.join(profile, 'devices.json'), 'utf8'), saved);
    console.log('PASS real per-user installation ' + (iteration + 1) + ': every payload hash matches and saved settings survive.');
  }
  await run(process.execPath, ['tests/packaged-smoke.cjs'], { ...process.env, REMOTECONTROL_PACKAGED_EXE: path.join(installed, 'RemoteControl.exe') });
  await run(path.join(installed, 'Uninstall RemoteControl.exe'), ['/S', '_?=' + installed]);
  assert.equal(await fs.access(path.join(installed, 'RemoteControl.exe')).then(() => true, () => false), false);
  assert.equal(await fs.readFile(path.join(profile, 'devices.json'), 'utf8'), saved);
  console.log('PASS installed production UI/security smoke and uninstall; settings preserved. No remote engine executed.');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => {
  if (profile && path.resolve(profile) === path.resolve(process.env.APPDATA, 'remotecontrol-desktop')) await fs.rm(profile, { recursive: true, force: true });
  if (temporary && path.resolve(temporary).startsWith(path.resolve(os.tmpdir()) + path.sep + 'remotecontrol-installer-test-')) await fs.rm(temporary, { recursive: true, force: true });
});
