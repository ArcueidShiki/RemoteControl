'use strict';
// An explicit integration check, excluded from packages. Default: verify only.
// The optional version probe runs ONLY on a disposable GitHub-hosted Windows
// runner: upstream's portable wrapper extracts into the Windows user profile.
// It never receives a peer ID, connection flag, credentials or installation flag.
const fs = require('node:fs/promises');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { EventEmitter, once } = require('node:events');
const assert = require('node:assert/strict');
const { DeviceStore, EngineLauncher } = require('../src/core.cjs');
const { verifyEngine, engineProcessOptions } = require('../src/engine.cjs');
const root = path.resolve(__dirname, '..');
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
async function waitForPhase(launcher, phase) {
  const deadline = Date.now() + 10000;
  while (launcher.state.phase === 'preparing' && Date.now() < deadline) await pause(20);
  assert.equal(launcher.state.phase, phase, launcher.state.message);
}
(async () => {
  const source = process.argv[2];
  if (!source) throw new Error('Pass an absolute path to the separately obtained official RustDesk release.');
  const identity = await verifyEngine(source); assert.equal(identity.verified, true, identity.message);
  await fs.mkdir(path.join(root, '.userdata'), { recursive: true });
  const profile = await fs.mkdtemp(path.join(root, '.userdata', 'trusted-engine-'));
  const executable = path.join(profile, 'rustdesk.exe'); await fs.copyFile(source, executable);
  const cwd = path.join(profile, 'engine-working-directory'); await fs.mkdir(cwd);
  const store = new DeviceStore(path.join(profile, 'devices.json')); await store.setEnginePath(executable);
  const restored = new DeviceStore(store.file); await restored.load();
  assert.equal((await verifyEngine(restored.data.enginePath)).verified, true);
  console.log('PASS official release digest and restored saved path verified: ' + identity.sha256);
  const device = { name: 'No remote launch - captured arguments only', peerId: '123456789' };
  const captured = [];
  const launcher = new EngineLauncher({ delay: 60, workingDirectory: cwd, spawn: (...args) => {
    captured.push(args); const child = new EventEmitter(); child.unref = () => {};
    setImmediate(() => child.emit('spawn')); return child;
  } });
  launcher.prepare(device, 'desktop', true, executable); launcher.cancel(); await pause(100); assert.equal(captured.length, 0);
  for (const action of ['desktop', 'files', 'desktop']) {
    launcher.prepare(device, action, true, executable); await waitForPhase(launcher, 'handed-off');
    const invocation = captured.at(-1);
    assert.deepEqual(invocation[1], [action === 'files' ? '--file-transfer' : '--connect', device.peerId]);
    assert.equal(invocation[2].cwd, cwd); assert.equal(invocation[2].shell, false);
    launcher.reset(true);
  }
  console.log('PASS real digest verification with captured desktop/file/repeated handoff arguments and cancel; no remote command executed.');
  launcher.prepare(device, 'desktop', true, executable);
  const changed = await fs.open(executable, 'r+');
  try { await changed.write(Buffer.from([0]), 0, 1, 0); } finally { await changed.close(); }
  await waitForPhase(launcher, 'error'); assert.equal(launcher.state.engineInvalid, true); assert.equal(captured.length, 3);
  assert.equal((await verifyEngine(restored.data.enginePath)).verified, false);
  launcher.dispose();
  console.log('PASS same-size replacement after selection/prepare invalidates saved identity and blocks launch.');
  if (process.argv.includes('--probe-version')) {
    if (process.env.GITHUB_ACTIONS !== 'true' || process.env.RUNNER_ENVIRONMENT !== 'github-hosted' || process.platform !== 'win32')
      throw new Error('Version probe requires a disposable GitHub-hosted Windows runner; the wrapper touches its user profile.');
    // Recheck the unmodified source immediately before this actual invocation.
    assert.equal((await verifyEngine(source)).verified, true);
    const child = spawn(source, ['--version'], engineProcessOptions(cwd, { windowsHide: true, detached: false, stdio: ['ignore', 'pipe', 'pipe'] }));
    let output = ''; const append = value => { output = (output + value).slice(-16384); };
    child.stdout.on('data', append); child.stderr.on('data', append);
    const timeout = setTimeout(() => child.kill(), 30000);
    let code;
    try { [code] = await once(child, 'close'); } finally { clearTimeout(timeout); }
    assert.equal(code, 0, output); assert.match(output, /(?:^|\r?\n)1\.5\.0(?:\r?\n|$)/, output);
    console.log('PASS actual trusted RustDesk --version invocation returned 1.5.0 using the sanitized environment and deliberate cwd. No session tested.');
  } else console.log('VERIFY ONLY: no actual RustDesk process started on this desktop.');
})().catch(error => { console.error(error); process.exitCode = 1; });
