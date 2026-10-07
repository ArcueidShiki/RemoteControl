'use strict';
const { _electron: electron, chromium } = require('playwright-core');
const fs = require('node:fs/promises');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const evidence = path.join(root, 'output', 'playwright');
const profile = path.join(root, '.userdata', 'ui-' + Date.now());
let application, browser, checks = 0;
const browserUrl = process.env.REMOTECONTROL_BROWSER_URL;
if (browserUrl && new URL(browserUrl).hostname !== '127.0.0.1') throw new Error('Browser fixture must use loopback.');
function check(value, message) { assert.ok(value, message); checks++; console.log('PASS ' + message); }
async function waitFor(fn, message, timeout = 10000) {
  const end = Date.now() + timeout;
  while (Date.now() < end) { if (await fn()) { check(true, message); return; } await new Promise(r => setTimeout(r, 100)); }
  throw new Error(message);
}
(async () => {
  await fs.mkdir(evidence, { recursive: true }); await fs.mkdir(profile, { recursive: true });
  await fs.writeFile(path.join(profile, 'devices.json'), JSON.stringify({ devices: [], enginePath: process.execPath }));
  let page;
  if (browserUrl) {
    browser = await chromium.launch({ channel: 'chrome', headless: true, chromiumSandbox: true });
    page = await browser.newPage({ viewport: { width: 1200, height: 830 } });
    await page.goto(browserUrl);
    await page.evaluate(async () => { const state = await window.remoteControl.snapshot(); if (!state.value.testMode) throw new Error('Not a test fixture'); for (const device of state.value.devices) await window.remoteControl.removeDevice(device.id); });
    await page.reload();
  } else {
    application = await electron.launch({ executablePath: process.env.REMOTECONTROL_ELECTRON || undefined, args: [root],
      env: { ...process.env, REMOTECONTROL_TEST: '1', REMOTECONTROL_PROFILE: profile }, timeout: 30000 });
    page = await application.firstWindow();
  }
  const errors = []; page.on('pageerror', error => errors.push(error.message));
  await page.locator('#local-name').waitFor();
  await waitFor(async () => (await page.locator('#engine-summary').textContent()).includes('RustDesk'), browserUrl ? 'browser fixture bridge initializes (not Electron IPC)' : 'actual Electron preload/main bridge initializes');
  check(await page.evaluate(() => typeof window.require === 'undefined'), 'renderer has no Node require');
  if (application) check(await application.evaluate(({ BrowserWindow }) => BrowserWindow.getAllWindows()[0].webContents.getLastWebPreferences().sandbox), 'renderer sandbox remains enabled');
  await require('./wizard-flow.cjs')(page, evidence, check);
  check(await page.locator('#connect').isDisabled(), 'no computer or consent cannot connect');
  await page.screenshot({ path: path.join(evidence, 'cross-platform-empty.png') });
  await page.locator('#add-device').click();
  await page.locator('#device-name').fill('Studio workstation');
  await page.locator('#device-id').fill('192.168.1.20');
  await page.getByRole('button', { name: '保存电脑', exact: true }).click();
  await waitFor(async () => (await page.locator('#device-error').textContent()).includes('6–12'), 'direct IP cannot bypass the ID route');
  await page.locator('#device-id').fill('123 456 789');
  await page.locator('#device-ip').fill('192.168.1.20');
  await page.getByRole('button', { name: '保存电脑', exact: true }).click();
  await waitFor(async () => await page.locator('#device-dialog').evaluate(el => !el.open), browserUrl ? 'device saves through the loopback fixture API' : 'device saves through actual IPC');
  check((await page.locator('#peer-id').textContent()) === '123456789', 'normalized peer ID displayed');
  check((await page.locator('#peer-ip').textContent()).includes('（备注）'), 'peer IP is explicitly an unverified label');
  check(await page.locator('#connect').isDisabled(), 'consent defaults off for saved device');
  await page.locator('#consent').check();
  check(await page.locator('#connect').isEnabled(), 'explicit consent enables handoff');
  await page.screenshot({ path: path.join(evidence, 'cross-platform-computers.png') });
  await page.locator('#connect').click();
  await page.locator('#cancel-handoff').click();
  await waitFor(async () => (await page.locator('#session-description').textContent()).includes('已取消'), 'cancel before launch prevents handoff');
  await new Promise(r => setTimeout(r, 600));
  check((await page.locator('#session-badge').textContent()) === '未连接', 'late timer cannot revive cancellation');
  for (const mode of ['desktop', 'files', 'desktop']) {
    await page.locator('#action').selectOption(mode); await page.locator('#consent').check();
    await page.locator('#connect').click();
    await waitFor(async () => (await page.locator('#session-badge').textContent()) === '状态未知', 'repeated desktop/file handoff completes');
    check((await page.locator('#session-description').textContent()).includes('无法获知'), 'handoff does not claim authentication or connection success');
    check(await page.locator('#cancel-handoff').isHidden(), 'post-handoff cancellation does not falsely claim to disconnect');
    if (mode === 'files') await page.screenshot({ path: path.join(evidence, 'cross-platform-handoff.png') });
    await page.locator('#reset-handoff').click();
    await waitFor(async () => (await page.locator('#session-badge').textContent()) === '未连接' && !(await page.locator('#consent').isChecked()), 'new attempt requires fresh consent');
  }
  await page.locator('#search').fill('does not exist');
  check((await page.locator('#device-list').textContent()).includes('没有匹配'), 'search handles no results');
  await page.locator('#search').fill('');
  await page.locator('.nav[data-page="settings"]').click();
  await page.screenshot({ path: path.join(evidence, 'cross-platform-settings.png') });
  check((await page.locator('#settings-page').textContent()).includes('未随本应用提供'), 'external engine boundary visible in settings');
  await page.locator('summary').click();
  await page.locator('[data-page="lab"]').click();
  // Interrupt negotiation immediately; repeat start/stop to reject stale callbacks.
  await page.locator('#lab-start').click(); await page.locator('#lab-stop').click();
  check(await page.locator('#lab-start').isEnabled(), 'local peer negotiation cancels');
  const measurements = [];
  for (const height of ['1080', '1440', '2160']) {
    await page.locator('#lab-resolution').selectOption(height); await page.locator('#lab-start').click();
    await waitFor(async () => (await page.locator('#lab-state').textContent()).includes('DTLS connected'), 'actual local WebRTC peers establish DTLS', 20000);
    await waitFor(async () => Number(await page.locator('#metric-fps').textContent()) > 0, 'actual decoded frame counter produces FPS', 15000);
    await page.locator('#lab-ping').click();
    await waitFor(async () => (await page.locator('#lab-echo').textContent()).includes('pointer event acknowledged'), 'pointer event traverses the real data channel');
    await page.locator('#lab-keyboard').focus(); await page.keyboard.press('A');
    await waitFor(async () => (await page.locator('#lab-echo').textContent()).includes('key event acknowledged'), 'keyboard event traverses the real data channel without character content');
    const samples = [];
    for (let i = 0; i < 5; i++) {
      await new Promise(r => setTimeout(r, 1100));
      await page.locator('#lab-ping').click();
      samples.push(await page.evaluate(() => ({ received: document.getElementById('metric-resolution').textContent,
        decodedFps: Number(document.getElementById('metric-fps').textContent), echo: document.getElementById('metric-rtt').textContent,
        codec: document.getElementById('metric-codec').textContent, videoWidth: document.getElementById('lab-video').videoWidth, videoHeight: document.getElementById('lab-video').videoHeight })));
    }
    check(samples.every(s => s.videoWidth > 0 && s.videoHeight > 0 && s.decodedFps > 0), 'requested resolution produces real decoded frames; report actual dimensions');
    measurements.push({ requested: { width: Number(height) * 16 / 9, height: Number(height), fps: 30 }, samples });
    await page.evaluate(() => window.scrollTo(0, 0));
    await page.screenshot({ path: path.join(evidence, 'cross-platform-lab-' + height + '.png'), fullPage: true });
    await page.locator('#lab-stop').click();
    check(await page.locator('#lab-video').evaluate(video => video.srcObject === null), 'stopping releases the video source');
    check(await page.locator('#lab-ping').isDisabled(), 'stopping disables input echo');
  }
  await fs.writeFile(path.join(evidence, 'webrtc-measurements.json'), JSON.stringify({ description: 'Synthetic canvas -> local WebRTC -> video decode, not RustDesk or remote desktop performance.', versions: application ? await application.evaluate(() => process.versions) : { browser: await browser.version() }, measurements }, null, 2));
  await page.locator('[data-page="devices"]').click();
  await page.locator('#add-device').click(); await page.locator('#device-name').fill('<img src=x onerror=alert(1)>');
  await page.locator('#device-id').fill('987654321'); await page.getByRole('button', { name: '保存电脑', exact: true }).click();
  await waitFor(async () => (await page.locator('#device-count').textContent()) === '2', 'second device saves');
  check(await page.locator('#device-list img').count() === 0, 'device names are rendered as text, not HTML');
  await page.getByRole('button', { name: '移除 <img src=x onerror=alert(1)>', exact: true }).click();
  await waitFor(async () => (await page.locator('#device-count').textContent()) === '1', 'removing a device updates the workspace');
  check(errors.length === 0, 'no renderer exceptions: ' + errors.join('; '));
  console.log('ALL ' + checks + (browserUrl ? ' BROWSER FIXTURE' : ' ELECTRON') + ' UI CHECKS PASSED; synthetic/local peers, mocked engine launch.');
})().catch(error => { console.error(error); process.exitCode = 1; })
  .finally(async () => { if (application) await application.close(); if (browser) await browser.close(); });
