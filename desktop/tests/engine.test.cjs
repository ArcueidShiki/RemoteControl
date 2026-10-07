'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const { DeviceStore, EngineLauncher } = require('../src/core.cjs');
const { RELEASES, verifyEngine, engineEnvironment, engineProcessOptions } = require('../src/engine.cjs');
const device = { name: 'Fixture only', peerId: '123456789' };
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
async function temporary(fn) {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-engine-test-'));
  try { await fn(directory); } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-engine-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
}
test('saved paths and renamed executables do not establish engine trust', () => temporary(async directory => {
  const executable = path.join(directory, 'rustdesk.exe');
  await fs.copyFile(process.execPath, executable);
  const store = new DeviceStore(path.join(directory, 'devices.json'));
  store.data.enginePath = executable; await store.save();
  const restored = new DeviceStore(store.file); await restored.load();
  assert.equal(restored.data.enginePath, executable);
  assert.equal((await verifyEngine(restored.data.enginePath, { platform: 'win32', arch: 'x64' })).verified, false);
  assert.equal((await verifyEngine(process.execPath)).ready, false);
  assert.equal((await verifyEngine(executable, { platform: 'darwin', arch: 'arm64' })).ready, false);
  let launches = 0;
  const launcher = new EngineLauncher({ workingDirectory: directory, delay: 1, spawn: () => { launches++; } });
  const done = once(launcher, 'state'); // preparing is synchronous
  launcher.prepare(device, 'desktop', true, restored.data.enginePath); await done;
  while (launcher.state.phase === 'preparing') await pause(10);
  assert.equal(launcher.state.phase, 'error'); assert.equal(launcher.state.engineInvalid, true); assert.equal(launches, 0);
  launcher.dispose();
}));
test('matching size and filename still require the pinned content digest', () => temporary(async directory => {
  const executable = path.join(directory, 'rustdesk.exe');
  await fs.writeFile(executable, Buffer.alloc(RELEASES['win32-x64'].size));
  const result = await verifyEngine(executable, { platform: 'win32', arch: 'x64' });
  assert.equal(result.verified, false); assert.match(result.message, /digest/);
}));
test('cancel and close during asynchronous identity verification cannot launch or revive state', async () => {
  for (const action of ['cancel', 'dispose']) {
    let finish, started, launches = 0;
    const checking = new Promise(resolve => { started = resolve; });
    const launcher = new EngineLauncher({ workingDirectory: os.tmpdir(), delay: 1,
      verify: () => { started(); return new Promise(resolve => { finish = resolve; }); }, spawn: () => { launches++; } });
    launcher.prepare(device, 'desktop', true, process.execPath); await checking;
    launcher[action](); const state = launcher.state;
    finish({ ready: true, verified: true }); await pause(20);
    assert.equal(launches, 0); assert.equal(launcher.state, state);
  }
});
test('verification failures and changed identity remain blocked across repeated attempts', async () => {
  let checks = 0, launches = 0;
  const launcher = new EngineLauncher({ workingDirectory: os.tmpdir(), delay: 1,
    verify: async () => { checks++; return { ready: false, verified: false, message: 'Unverified: changed file.' }; }, spawn: () => { launches++; } });
  for (let i = 0; i < 3; i++) {
    launcher.prepare(device, 'desktop', true, process.execPath);
    while (launcher.state.phase === 'preparing') await pause(10);
    assert.equal(launcher.state.phase, 'error'); assert.match(launcher.state.message, /changed file/);
  }
  assert.equal(checks, 3); assert.equal(launches, 0); launcher.dispose();
});
test('OS environment allowlists omit secrets, shell settings, proxies and loader injection', () => {
  const hostile = { GITHUB_TOKEN: 'canary', AWS_SECRET_ACCESS_KEY: 'canary', NODE_OPTIONS: '--require=bad',
    ELECTRON_RUN_AS_NODE: '1', LD_PRELOAD: '/bad', DYLD_INSERT_LIBRARIES: '/bad', PYTHONPATH: '/bad',
    RUSTDESK_PASSWORD: 'canary', SSH_AUTH_SOCK: '/socket', HTTP_PROXY: 'http://bad', ComSpec: 'bad.exe',
    PATH: '/bad', PATHEXT: '.BAD', SystemRoot: 'C:\\Windows', APPDATA: 'C:\\Users\\Test\\AppData\\Roaming',
    HOME: '/Users/test', TMPDIR: '/tmp/', SECRET: 'canary' };
  const windows = engineEnvironment(hostile, 'win32');
  assert.deepEqual(Object.keys(windows).sort(), ['APPDATA', 'PATH', 'SystemRoot']);
  assert.equal(windows.PATH, 'C:\\Windows\\System32;C:\\Windows');
  assert.deepEqual(engineEnvironment(hostile, 'darwin'), { HOME: '/Users/test', TMPDIR: '/tmp/', PATH: '/usr/bin:/bin:/usr/sbin:/sbin' });
  assert.equal(engineEnvironment({ systemroot: 'C:\\Windows' }, 'win32').SystemRoot, 'C:\\Windows');
  assert.throws(() => engineEnvironment({ SystemRoot: 'relative' }, 'win32'));
  assert.throws(() => engineProcessOptions('relative'));
});
test('a real child receives the deliberate cwd and no parent secret or Node loader option', () => temporary(async directory => {
  const previousSecret = process.env.REMOTECONTROL_SECRET_CANARY, previousNode = process.env.NODE_OPTIONS;
  let child;
  try {
    process.env.REMOTECONTROL_SECRET_CANARY = 'never-forward'; process.env.NODE_OPTIONS = '--require=must-not-load';
    child = spawn(process.execPath, ['-e', 'process.stdout.write(JSON.stringify({cwd:process.cwd(),secret:process.env.REMOTECONTROL_SECRET_CANARY||null,node:process.env.NODE_OPTIONS||null}))'],
      engineProcessOptions(directory, { windowsHide: true, detached: false, stdio: ['ignore', 'pipe', 'pipe'] }));
  } finally {
    if (previousSecret === undefined) delete process.env.REMOTECONTROL_SECRET_CANARY; else process.env.REMOTECONTROL_SECRET_CANARY = previousSecret;
    if (previousNode === undefined) delete process.env.NODE_OPTIONS; else process.env.NODE_OPTIONS = previousNode;
  }
  let output = ''; child.stdout.on('data', value => { output += value; });
  const [code] = await once(child, 'close'); assert.equal(code, 0);
  // macOS exposes /var via /private/var; the child reports the physical cwd.
  assert.deepEqual(JSON.parse(output), { cwd: await fs.realpath(directory), secret: null, node: null });
}));
