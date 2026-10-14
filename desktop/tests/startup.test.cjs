'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const { EventEmitter } = require('node:events');

test('losing instance quits before any store access, IPC or window startup', () => {
  const calls = [], app = new EventEmitter();
  app.isPackaged = true;
  app.setPath = (name, value) => { calls.push(['profile', name, value]); };
  app.requestSingleInstanceLock = () => { calls.push(['lock']); return false; };
  app.quit = () => { calls.push(['quit']); };
  const forbidden = () => { throw new Error('Secondary instance entered persistent/startup work.'); };
  app.whenReady = forbidden; app.getPath = forbidden;
  const profile = path.resolve(__dirname, 'fixture-profile');
  const source = fs.readFileSync(path.join(__dirname, '..', 'src', 'main.cjs'), 'utf8');
  vm.runInNewContext(source, {
    __dirname: path.join(__dirname, '..', 'src'),
    process: { env: { REMOTECONTROL_PROFILE: profile }, platform: process.platform },
    require: name => {
      if (name === 'electron') return { app, BrowserWindow: forbidden, ipcMain: { handle: forbidden }, protocol: { registerSchemesAsPrivileged: forbidden } };
      if (name === './core.cjs') return { DeviceStore: forbidden, EngineLauncher: forbidden };
      if (name === './engine.cjs') return { verifyEngine: forbidden };
      if (name === './network.cjs') return { describeInterfaces: forbidden };
      if (name === 'node:fs/promises') return { readFile: forbidden, writeFile: forbidden, mkdir: forbidden };
      return require(name);
    }
  }, { filename: 'main.cjs' });
  assert.deepEqual(calls, [['profile', 'userData', profile], ['lock'], ['quit']]);
  assert.equal(app.listenerCount('second-instance'), 0);
});
