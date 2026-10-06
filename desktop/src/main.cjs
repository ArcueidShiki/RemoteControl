'use strict';
const { app, BrowserWindow, ipcMain, dialog, shell, protocol, net, session } = require('electron');
const path = require('node:path');
const os = require('node:os');
const fs = require('node:fs/promises');
const { pathToFileURL } = require('node:url');
const { DeviceStore, EngineLauncher } = require('./core.cjs');

const testing = process.env.REMOTECONTROL_TEST === '1' && !app.isPackaged;
if (process.env.REMOTECONTROL_PROFILE) app.setPath('userData', path.resolve(process.env.REMOTECONTROL_PROFILE));
protocol.registerSchemesAsPrivileged([{ scheme: 'workspace', privileges: { standard: true, secure: true, supportFetchAPI: true } }]);
let window, store, launcher;
function localAddresses() {
  return Object.values(os.networkInterfaces()).flat().filter(x => x && !x.internal && x.family === 'IPv4').map(x => x.address);
}
async function engineStatus() {
  const executable = store.data.enginePath;
  try { const stat = await fs.stat(executable); if (stat.isFile()) return { ready: true, path: executable }; } catch {}
  return { ready: false, path: executable };
}
async function snapshot() {
  return { devices: store.data.devices, engine: await engineStatus(), local: { name: os.hostname(), platform: process.platform, addresses: localAddresses() },
    session: launcher.state, loadError: store.loadError || '', testMode: testing };
}
function handler(channel, fn) {
  ipcMain.handle(channel, async (event, ...args) => {
    if (!window || event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame || event.senderFrame.url !== 'workspace://app/index.html')
      throw new Error('Untrusted request.');
    try { return { ok: true, value: await fn(...args) }; }
    catch (error) { return { ok: false, error: error.message || 'The action could not be completed.' }; }
  });
}
app.whenReady().then(async () => {
  protocol.handle('workspace', request => {
    const url = new URL(request.url);
    const allowed = new Set(['/index.html', '/app.js', '/style.css']);
    if (url.hostname !== 'app' || !allowed.has(url.pathname)) return new Response('Not found', { status: 404 });
    return net.fetch(pathToFileURL(path.join(__dirname, url.pathname.slice(1))).href);
  });
  session.defaultSession.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  session.defaultSession.setPermissionCheckHandler(() => false);
  // The renderer serves packaged assets only. No tracker, remote font, scan,
  // network media capture, clipboard API or arbitrary shell bridge.
  session.defaultSession.webRequest.onBeforeRequest((details, callback) => callback({ cancel: !details.url.startsWith('workspace://app/') && !details.url.startsWith('file://') }));
  store = new DeviceStore(path.join(app.getPath('userData'), 'devices.json'));
  await store.load();
  if (testing) {
    const { EventEmitter } = require('node:events');
    launcher = new EngineLauncher({ delay: 1200, spawn: () => {
      const child = new EventEmitter(); child.unref = () => {};
      setImmediate(() => child.emit(process.env.REMOTECONTROL_TEST_FAIL_ENGINE ? 'error' : 'spawn', new Error('fixture')));
      return child;
    } });
  } else launcher = new EngineLauncher();
  window = new BrowserWindow({ width: 1200, height: 830, minWidth: 900, minHeight: 690, title: 'RemoteControl', backgroundColor: '#f5f7fb', show: !testing,
    webPreferences: { preload: path.join(__dirname, 'preload.cjs'), nodeIntegration: false, contextIsolation: true, sandbox: true, webSecurity: true, backgroundThrottling: !testing, devTools: !app.isPackaged } });
  window.setMenuBarVisibility(false);
  window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  window.webContents.on('will-navigate', event => event.preventDefault());
  launcher.on('state', state => { if (!window.isDestroyed()) window.webContents.send('workspace:session', state); });
  handler('workspace:snapshot', snapshot);
  handler('workspace:save-device', async value => { await store.put(value); return snapshot(); });
  handler('workspace:remove-device', async id => { if (launcher.state.phase === 'preparing' || launcher.state.phase === 'handed-off') throw new Error('Finish the current handoff first.'); await store.remove(String(id)); return snapshot(); });
  handler('workspace:choose-engine', async () => {
    if (launcher.state.phase === 'preparing' || launcher.state.phase === 'handed-off') throw new Error('Finish the current handoff first.');
    const result = await dialog.showOpenDialog(window, { title: 'Choose the official RustDesk application', properties: ['openFile'], filters: process.platform === 'win32' ? [{ name: 'RustDesk executable', extensions: ['exe'] }] : [] });
    if (result.canceled) return snapshot();
    let executable = result.filePaths[0];
    if (process.platform === 'darwin' && executable.endsWith('.app')) executable = path.join(executable, 'Contents', 'MacOS', 'RustDesk');
    if (!/^rustdesk(?:[-\w.]*)?(?:\.exe)?$/i.test(path.basename(executable))) throw new Error('Select the official RustDesk executable. Only RustDesk is supported.');
    if (!(await fs.stat(executable)).isFile()) throw new Error('RustDesk executable was not found.');
    store.data.enginePath = executable; await store.save(); return snapshot();
  });
  handler('workspace:prepare', async value => {
    if (!(await engineStatus()).ready) throw new Error('Choose the official RustDesk application in Settings first.');
    const device = store.data.devices.find(d => d.id === value?.id);
    if (!device) throw new Error('Save or choose a computer first.');
    return launcher.prepare(device, value.action, value.consent, store.data.enginePath);
  });
  handler('workspace:cancel', () => launcher.cancel());
  handler('workspace:reset', confirmed => launcher.reset(confirmed));
  handler('workspace:documentation', () => shell.openExternal('https://rustdesk.com/docs/en/'));
  window.on('close', event => {
    if (launcher.state.phase === 'handed-off' && !testing) {
      const choice = dialog.showMessageBoxSync(window, { type: 'info', buttons: ['Keep open', 'Close workspace'], defaultId: 0, cancelId: 0,
        message: 'A RustDesk session may still be open.', detail: 'Closing this workspace does not disconnect RustDesk. End the remote session in RustDesk first.' });
      if (choice === 0) event.preventDefault();
    }
  });
  await window.loadURL('workspace://app/index.html');
});
app.on('window-all-closed', () => app.quit());
app.on('before-quit', () => launcher?.dispose());
