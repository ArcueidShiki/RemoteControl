'use strict';
const { _electron: electron } = require('playwright-core');
const fs = require('node:fs/promises');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
let application;
(async () => {
  await fs.mkdir(path.join(root, '.userdata'), { recursive: true });
  const profile = await fs.mkdtemp(path.join(root, '.userdata', 'packaged-'));
  const evidence = path.join(root, 'output', 'playwright'); await fs.mkdir(evidence, { recursive: true });
  const impostor = path.join(profile, 'rustdesk.exe');
  await fs.writeFile(impostor, 'This is a test fixture, not an executable.');
  await fs.writeFile(path.join(profile, 'devices.json'), JSON.stringify({ devices: [{ name: 'Fixture - no remote session', peerId: '123456789' }], enginePath: impostor }));
  // Deliberately set the development mock flag: packaged builds MUST ignore it.
  application = await electron.launch({ executablePath: path.join(root, 'dist', 'RemoteControl-win32-x64', 'RemoteControl.exe'),
    args: [], env: { ...process.env, REMOTECONTROL_PROFILE: profile, REMOTECONTROL_TEST: '1' }, timeout: 30000 });
  const page = await application.firstWindow(); const errors = []; page.on('pageerror', error => errors.push(error.message));
  await page.waitForFunction(() => document.getElementById('engine-summary').textContent.includes('Unverified'));
  assert.equal(await application.evaluate(({ app }) => app.isPackaged), true);
  const preferences = await application.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].webContents.getLastWebPreferences());
  assert.equal(preferences.sandbox, true); assert.equal(preferences.contextIsolation, true); assert.equal(preferences.nodeIntegration, false);
  assert.equal(await page.evaluate(() => typeof window.require), 'undefined');
  const snapshot = await page.evaluate(() => window.remoteControl.snapshot());
  assert.equal(snapshot.ok, true); assert.equal(snapshot.value.testMode, false);
  assert.equal(snapshot.value.engine.ready, false); assert.equal(snapshot.value.engine.verified, false);
  await page.getByRole('button', { name: 'Select Fixture - no remote session', exact: true }).click();
  await page.locator('#consent').check(); assert.equal(await page.locator('#connect').isDisabled(), true);
  // Renderer manipulation/direct IPC also cannot bypass launch-time checking.
  await page.evaluate(() => window.remoteControl.prepare({ id: '123456789', action: 'desktop', consent: true }));
  await page.waitForFunction(() => document.getElementById('session-description').textContent.includes('Unverified'));
  assert.equal((await page.evaluate(() => window.remoteControl.snapshot())).value.session.phase, 'error');
  await page.locator('[data-page="settings"]').click();
  assert.match(await page.locator('#engine-verification').textContent(), /Unverified/);
  await page.screenshot({ path: path.join(evidence, 'packaged-unverified-engine.png') });
  assert.deepEqual(errors, []);
  console.log('PASS packaged ASAR startup, real IPC, production verification, saved impostor blocked, test flag ignored, sandbox/isolation enabled, Settings screenshot. No engine launched.');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => { if (application) await application.close(); });
