'use strict';
// Native GUI regression for a private/CI desktop only; never run concurrently
// with someone controlling shared desktop windows. No remote engine is used.
const { _electron: electron } = require('playwright-core');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs/promises');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const executable = path.join(root, 'dist', 'RemoteControl-win32-x64', 'RemoteControl.exe');
let application;
const children = new Set();
async function waitFor(fn, message) {
  const deadline = Date.now() + 15000;
  while (Date.now() < deadline) {
    if (await fn()) return;
    await new Promise(resolve => setTimeout(resolve, 100));
  }
  throw new Error(message);
}
async function open(profile) {
  application = await electron.launch({ executablePath: executable, args: [],
    env: { ...process.env, REMOTECONTROL_PROFILE: profile }, timeout: 30000 });
  const page = await application.firstWindow();
  await page.waitForFunction(() => document.getElementById('engine-summary').textContent.includes('未通过校验'));
  if (await page.locator('#setup-dialog').evaluate(el => el.open)) await page.locator('#wizard-close').click();
  assert.equal(await application.evaluate(({ app }) => app.isPackaged && app.hasSingleInstanceLock()), true);
  return page;
}
function secondary(profile) {
  // Incoming arguments must not trigger a session in the primary instance.
  const child = spawn(executable, ['--connect', '123456789'], { shell: false, windowsHide: true,
    stdio: 'ignore', env: { ...process.env, REMOTECONTROL_PROFILE: profile } });
  children.add(child);
  const ended = once(child, 'exit');
  // Attach a rejection handler immediately, even while a concurrent save runs.
  ended.catch(() => {});
  const done = (async () => {
    let timeout;
    try {
      const [code, signal] = await Promise.race([ended, new Promise((_, reject) => {
        timeout = setTimeout(() => reject(new Error('Second packaged instance did not exit.')), 15000);
      })]);
      assert.equal(code, 0); assert.equal(signal, null);
      children.delete(child);
    } finally { clearTimeout(timeout); }
  })();
  done.catch(() => {});
  return done;
}
async function save(page, name, peerId) {
  const result = await page.evaluate(value => window.remoteControl.saveDevice(value), { name, peerId });
  assert.equal(result.ok, true, result.error);
}
(async () => {
  await fs.mkdir(path.join(root, '.userdata'), { recursive: true });
  const profile = await fs.mkdtemp(path.join(root, '.userdata', 'single-instance-'));
  const dataFile = path.join(profile, 'devices.json');
  const enginePath = path.join(profile, 'rustdesk.exe');
  await fs.writeFile(enginePath, 'Unverified fixture. Never executed.');
  await fs.writeFile(dataFile, JSON.stringify({ devices: [], enginePath }));
  let page = await open(profile);
  await application.evaluate(({ app, BrowserWindow }) => {
    globalThis.instanceEvents = 0; globalThis.focusRequests = 0;
    app.on('second-instance', () => { globalThis.instanceEvents++; });
    const window = BrowserWindow.getAllWindows()[0], focus = window.focus.bind(window);
    window.focus = () => { globalThis.focusRequests++; focus(); };
  });
  await save(page, 'Saved before repeated launches', '123456789');
  const expectedIds = ['123456789'];
  for (const [index, mode] of ['minimized', 'hidden'].entries()) {
    await application.evaluate(({ BrowserWindow }, state) => {
      const window = BrowserWindow.getAllWindows()[0];
      if (state === 'minimized') window.minimize(); else window.hide();
    }, mode);
    await waitFor(() => application.evaluate(({ BrowserWindow }, state) => {
      const window = BrowserWindow.getAllWindows()[0];
      return state === 'minimized' ? window.isMinimized() : !window.isVisible();
    }, mode), 'Primary did not enter the requested window state.');
    const done = secondary(profile);
    const peerId = String(234567890 + index); expectedIds.push(peerId);
    // Save while the competing process starts. Only the owning process may
    // open the store; the competitor must exit instead of keeping stale data.
    await save(page, 'Saved during ' + mode + ' relaunch', peerId);
    await done;
    await waitFor(() => application.evaluate(({ BrowserWindow }, count) => {
      const windows = BrowserWindow.getAllWindows(), window = windows[0];
      return windows.length === 1 && globalThis.instanceEvents === count && globalThis.focusRequests >= count &&
        window.isVisible() && !window.isMinimized();
    }, index + 1), 'Existing window did not restore/show and request focus.');
    const disk = JSON.parse(await fs.readFile(dataFile, 'utf8'));
    assert.deepEqual(disk.devices.map(device => device.peerId).sort(), [...expectedIds].sort());
    assert.equal(disk.enginePath, enginePath);
    const state = (await page.evaluate(() => window.remoteControl.snapshot())).value;
    assert.equal(state.session.phase, 'idle'); assert.equal(state.engine.ready, false);
    assert.equal(await fs.stat(dataFile + '.tmp').then(() => true, error => { if (error.code === 'ENOENT') return false; throw error; }), false);
    console.log('PASS second packaged process exits, ' + mode + ' primary restores/focuses, concurrent save preserves all devices and engine path, no session starts.');
  }
  await application.close(); application = null;
  page = await open(profile);
  const restored = (await page.evaluate(() => window.remoteControl.snapshot())).value;
  assert.deepEqual(restored.devices.map(device => device.peerId).sort(), [...expectedIds].sort());
  assert.equal(restored.engine.path, enginePath);
  await save(page, 'Saved after owner restarts', '456789012'); expectedIds.push('456789012');
  const finalStore = JSON.parse(await fs.readFile(dataFile, 'utf8'));
  assert.deepEqual(finalStore.devices.map(device => device.peerId).sort(), [...expectedIds].sort());
  assert.equal(finalStore.enginePath, enginePath);
  await page.reload(); await page.waitForFunction(() => document.getElementById('device-count').textContent === '4');
  await fs.mkdir(path.join(root, 'output', 'playwright'), { recursive: true });
  await page.screenshot({ path: path.join(root, 'output', 'playwright', 'packaged-single-instance.png') });
  console.log('PASS owner restart reacquires the lock, reloads all saved devices/path and persists a further edit. Packaged single-instance regression passed; no engine launched.');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => {
  for (const child of children) child.kill();
  if (application) await application.close();
});
