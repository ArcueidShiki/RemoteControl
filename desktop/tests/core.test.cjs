'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { EventEmitter } = require('node:events');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { cleanDevice, launchArguments, DeviceStore, EngineLauncher } = require('../src/core.cjs');
const device = { name: 'My studio', peerId: '123 456 789', peerIp: '192.168.1.20' };
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const fixtureOptions = { workingDirectory: os.tmpdir(), verify: async () => ({ ready: true, verified: true }) };
test('device validation prevents command/URI injection and direct-IP launch', () => {
  assert.equal(cleanDevice(device).peerId, '123456789');
  for (const peerId of ['--password', '123&password=secret', '192.168.1.1', '123456;calc', '../123456', 'rustdesk://123456', '1', '1234567890123'])
    assert.throws(() => cleanDevice({ ...device, peerId }), { code: 'INVALID_PEER_ID' });
  assert.throws(() => cleanDevice({ ...device, peerIp: 'https://example.test' }));
  assert.throws(() => cleanDevice({ ...device, name: '' }));
  assert.deepEqual(launchArguments(device, 'desktop'), ['--connect', '123456789']);
  assert.deepEqual(launchArguments(device, 'files'), ['--file-transfer', '123456789']);
  assert.throws(() => launchArguments(device, 'terminal'));
});
test('permission is required; cancel and repeated cancel cannot spawn', async () => {
  const spawned = [];
  const engine = new EngineLauncher({ ...fixtureOptions, delay: 20, spawn: (...args) => spawned.push(args) });
  assert.throws(() => engine.prepare(device, 'desktop', false, process.execPath));
  assert.throws(() => engine.prepare(device, 'desktop', true, 'relative.exe'));
  engine.prepare(device, 'desktop', true, process.execPath);
  assert.throws(() => engine.prepare(device, 'desktop', true, process.execPath));
  engine.cancel();
  assert.throws(() => engine.cancel());
  await pause(50); assert.equal(spawned.length, 0); assert.equal(engine.state.phase, 'idle');
});
test('handoff uses shell-free bounded arguments and never reports a connection', async () => {
  let child, captured;
  const engine = new EngineLauncher({ ...fixtureOptions, delay: 5, spawn: (...args) => { captured = args; child = new EventEmitter(); child.unref = () => {}; setImmediate(() => child.emit('spawn')); return child; } });
  for (const action of ['desktop', 'files', 'desktop']) {
    engine.prepare(device, action, true, process.execPath); await pause(30);
    assert.equal(engine.state.phase, 'handed-off');
    assert.equal(captured[2].shell, false);
    assert.equal(captured[2].cwd, os.tmpdir());
    assert.notEqual(captured[2].env, process.env);
    assert.equal(captured[1].length, 2);
    assert.equal(captured[1][0], action === 'files' ? '--file-transfer' : '--connect');
    child.emit('exit', 0);
    assert.equal(engine.state.phase, 'handed-off'); // launcher may delegate to an existing process
    assert.throws(() => engine.cancel()); assert.throws(() => engine.reset(false));
    engine.reset(true);
  }
  engine.dispose();
});
test('failed engine launch can be retried; late events cannot revive reset state', async () => {
  let child;
  const engine = new EngineLauncher({ ...fixtureOptions, delay: 1, spawn: () => { child = new EventEmitter(); child.unref = () => {}; setImmediate(() => child.emit('error', new Error('ENOENT'))); return child; } });
  engine.prepare(device, 'desktop', true, process.execPath); await pause(25);
  assert.equal(engine.state.phase, 'error');
  engine.prepare(device, 'desktop', true, process.execPath); await pause(25);
  assert.equal(engine.state.phase, 'error');
  engine.reset(true); child.emit('spawn'); assert.equal(engine.state.phase, 'idle');
});
test('closing pending handoff stops the launch', async () => {
  let count = 0;
  const engine = new EngineLauncher({ ...fixtureOptions, delay: 20, spawn: () => { count++; } });
  engine.prepare(device, 'desktop', true, process.execPath); engine.dispose();
  await pause(50); assert.equal(count, 0);
});
test('device store deduplicates and persists only explicit metadata', async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-test-'));
  try {
    const file = path.join(directory, 'devices.json'), store = new DeviceStore(file);
    await store.load();
    await store.put({ ...device, password: 'never-save', clipboard: 'never-save' });
    await store.put({ ...device, name: 'Renamed computer' });
    const again = new DeviceStore(file); await again.load();
    assert.equal(again.data.devices.length, 1); assert.equal(again.data.devices[0].name, 'Renamed computer');
    assert.equal((await fs.readFile(file, 'utf8')).includes('never-save'), false);
    await again.remove('123456789'); assert.equal(again.data.devices.length, 0);
  } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
});
test('unreadable saved metadata is preserved instead of silently overwritten', async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-test-'));
  const file = path.join(directory, 'devices.json');
  try {
    await fs.writeFile(file, '{broken');
    const store = new DeviceStore(file); await store.load();
    assert.ok(store.loadError);
    await assert.rejects(store.put(device));
    assert.equal(await fs.readFile(file, 'utf8'), '{broken');
  } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
});
test('real write failures preserve committed state across cancel, reload and later saves', async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-test-'));
  const file = path.join(directory, 'devices.json'), temporary = file + '.tmp';
  try {
    const store = new DeviceStore(file); await store.load();
    await store.put(device); await store.setEnginePath('original-engine');
    const original = store.data, originalBytes = await fs.readFile(file, 'utf8');
    await fs.mkdir(temporary); // A real filesystem obstruction, not a mocked save response.
    for (const change of [
      () => store.put({ ...device, name: 'Canceled rename' }),
      () => store.put({ ...device, name: 'Canceled new device', peerId: '987987987' }),
      () => store.remove('123456789'),
      () => store.setEnginePath('canceled-engine')
    ]) {
      await assert.rejects(change(), error => error.code === 'SETTINGS_SAVE_FAILED' && !!error.cause.code);
      assert.equal(store.data, original, 'failed transactions do not publish a candidate snapshot');
      assert.equal(await fs.readFile(file, 'utf8'), originalBytes);
    }
    assert.throws(() => { store.data.devices.push(device); }, TypeError);
    assert.throws(() => { store.data.enginePath = 'bypass'; }, TypeError);
    await store.load(); assert.deepEqual(store.data, original);
    const reopened = new DeviceStore(file); await reopened.load(); assert.deepEqual(reopened.data, original);
    await fs.rmdir(temporary);
    await store.put({ ...device, name: 'Later successful device', peerId: '321321321' });
    await reopened.load();
    assert.deepEqual(reopened.data.devices.map(d => d.name), ['My studio', 'Later successful device']);
    assert.equal(reopened.data.enginePath, 'original-engine');
  } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
});
test('rename failure never commits candidate memory and recovers for the next transaction', async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-test-'));
  const file = path.join(directory, 'devices.json');
  try {
    const store = new DeviceStore(file); await store.load(); const original = store.data;
    await fs.mkdir(file); // Temporary write succeeds, but replacing a directory fails.
    await assert.rejects(store.put(device), { code: 'SETTINGS_SAVE_FAILED' });
    assert.equal(store.data, original);
    await assert.rejects(fs.stat(file + '.tmp'), { code: 'ENOENT' });
    await fs.rmdir(file); await store.put({ ...device, peerId: '987987987' });
    assert.deepEqual(store.data.devices.map(d => d.peerId), ['987987987']);
  } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
});
test('queued concurrent edits derive from the latest successful snapshot', async () => {
  const directory = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-test-'));
  try {
    const file = path.join(directory, 'devices.json'), store = new DeviceStore(file);
    await store.load();
    await Promise.all([store.put(device), store.put({ ...device, peerId: '321321321' }), store.setEnginePath('saved-engine')]);
    const reopened = new DeviceStore(file); await reopened.load();
    assert.deepEqual(reopened.data.devices.map(d => d.peerId), ['123456789', '321321321']);
    assert.equal(reopened.data.enginePath, 'saved-engine');
  } finally {
    assert.equal(path.dirname(path.resolve(directory)), path.resolve(os.tmpdir()));
    assert.ok(path.basename(directory).startsWith('remotecontrol-test-'));
    await fs.rm(directory, { recursive: true, force: true });
  }
});
