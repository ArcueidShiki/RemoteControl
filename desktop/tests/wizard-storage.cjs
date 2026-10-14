'use strict';
// Exercise real DeviceStore failures through the UI, using only this test's profile.
// Native Electron is the CI default; the loopback browser fixture supports local review.
const { _electron: electron, chromium } = require('playwright-core');
const { spawn } = require('node:child_process');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const evidence = path.join(root, 'output', 'playwright');
const browserMode = process.env.REMOTECONTROL_STORAGE_BROWSER === '1';
const saveError = '保存失败，原有设置未改变。请检查存储空间和写入权限后重试；也可以取消本次编辑。';
let application, browser, fixture, profile;
async function openNative() {
  application = await electron.launch({ executablePath: process.env.REMOTECONTROL_ELECTRON || undefined, args: [root],
    env: { ...process.env, REMOTECONTROL_TEST: '1', REMOTECONTROL_PROFILE: profile }, timeout: 30000 });
  return application.firstWindow();
}
async function peerStep(page, name, id) {
  await page.locator('#setup-dialog[open]').waitFor();
  await page.locator('#wizard-next').click();
  await page.locator('#wizard-refresh').click();
  await page.locator('#wizard-next').click();
  await page.locator('#wizard-name').fill(name); await page.locator('#wizard-id').fill(id);
  await page.locator('#wizard-next').click();
}
const snapshot = page => page.evaluate(async () => {
  const result = await window.remoteControl.snapshot();
  if (!result.ok || !result.value.testMode) throw new Error('Expected isolated test profile');
  return result.value;
});
(async () => {
  await fs.mkdir(path.join(root, '.userdata'), { recursive: true });
  profile = await fs.mkdtemp(path.join(root, '.userdata', 'storage-ui-'));
  await fs.mkdir(evidence, { recursive: true });
  const file = path.join(profile, 'devices.json'), temporary = file + '.tmp';
  const originalBytes = JSON.stringify({ devices: [], enginePath: process.execPath });
  await fs.writeFile(file, originalBytes);
  await fs.mkdir(temporary); // Real fs.writeFile failure, not HTTP/IPC failure injection.
  let page;
  if (browserMode) {
    fixture = spawn(process.execPath, [path.join(__dirname, 'browser-review.cjs'), profile], { cwd: root, stdio: ['ignore', 'pipe', 'pipe'], windowsHide: true });
    const url = await new Promise((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('Fixture startup timeout')), 10000);
      let output = '';
      fixture.stdout.on('data', chunk => { output += chunk; const match = output.match(/http:\/\/127\.0\.0\.1:\d+/); if (match) { clearTimeout(timer); resolve(match[0]); } });
      fixture.once('error', error => { clearTimeout(timer); reject(error); });
      fixture.once('exit', code => { clearTimeout(timer); reject(new Error('Fixture exited: ' + code)); });
    });
    browser = await chromium.launch({ channel: 'chrome', headless: true, chromiumSandbox: true });
    page = await browser.newPage({ viewport: { width: 900, height: 690 } }); await page.goto(url);
  } else page = await openNative();
  const errors = []; page.on('pageerror', error => errors.push(error.message));
  await peerStep(page, '取消的未保存电脑', '123123123');
  for (let attempt = 0; attempt < 2; attempt++) {
    await page.locator('#wizard-next').click();
    await page.waitForFunction(text => document.getElementById('wizard-error').textContent === text, saveError);
    assert.equal(await page.locator('#wizard-next').isEnabled(), true);
    assert.deepEqual((await snapshot(page)).devices, []);
    assert.equal(await fs.readFile(file, 'utf8'), originalBytes);
  }
  await page.screenshot({ path: path.join(evidence, 'wizard-real-storage-error.png') });
  await page.locator('#wizard-cancel').click();
  assert.deepEqual((await snapshot(page)).devices, []);
  await page.reload(); // Refresh from the same process must not reveal the failed edit.
  await peerStep(page, '稍后成功保存的电脑', '321321321');
  assert.deepEqual((await snapshot(page)).devices, []);
  await fs.rmdir(temporary); // Remove only this test-owned obstruction, then retry normally.
  await page.locator('#wizard-next').evaluate(button => { button.click(); button.click(); });
  await page.waitForFunction(() => !document.getElementById('setup-dialog').open);
  const state = await snapshot(page);
  assert.deepEqual(state.devices.map(d => d.peerId), ['321321321']);
  assert.equal(state.session.phase, 'idle'); assert.equal(await page.locator('#consent').isChecked(), false);
  assert.deepEqual(JSON.parse(await fs.readFile(file, 'utf8')).devices, state.devices);
  if (application) {
    await application.close(); application = undefined; page = await openNative();
  } else await page.reload();
  await page.locator('#local-name').waitFor();
  assert.deepEqual((await snapshot(page)).devices.map(d => d.peerId), ['321321321']);
  await page.waitForFunction(() => document.getElementById('device-count').textContent === '1');
  assert.equal(await page.locator('#setup-dialog').evaluate(dialog => dialog.open), false);
  await page.screenshot({ path: path.join(evidence, 'wizard-storage-recovered.png') });
  assert.deepEqual(errors, []);
  console.log('PASS ' + (browserMode ? 'browser loopback' : 'native Electron IPC/restart') + ': real filesystem failure, complete Chinese error, repeated failed save, cancel, refresh, later repeated save and disk reload; failed edit never persisted or appeared in memory.');
})().catch(error => { console.error(error); process.exitCode = 1; }).finally(async () => {
  await application?.close(); await browser?.close(); fixture?.kill();
  // Retain the isolated profile as inspectable evidence; never modify a daily profile.
});
