'use strict';
const { isIP } = require('node:net');
const fs = require('node:fs/promises');
const path = require('node:path');
const { EventEmitter } = require('node:events');
const { spawn } = require('node:child_process');
const { verifyEngine, engineProcessOptions } = require('./engine.cjs');

function cleanDevice(value) {
  if (!value || typeof value !== 'object') throw new Error('Enter a computer name and RustDesk ID.');
  const name = String(value.name || '').trim();
  const peerId = String(value.peerId || '').replace(/\s/g, '');
  const peerIp = String(value.peerIp || '').trim();
  if (!name || name.length > 60) throw new Error('Use a computer name between 1 and 60 characters.');
  if (!/^\d{6,12}$/.test(peerId)) throw new Error('Enter a RustDesk ID with 6–12 digits. Direct IP connections are not supported by this adapter.');
  if (peerIp && !isIP(peerIp)) throw new Error('Peer IP must be a valid IPv4 or IPv6 address, or left blank.');
  return { id: peerId, name, peerId, peerIp };
}
function launchArguments(device, action) {
  const checked = cleanDevice(device);
  if (!['desktop', 'files'].includes(action)) throw new Error('Unsupported session action.');
  return [action === 'files' ? '--file-transfer' : '--connect', checked.peerId];
}
class DeviceStore {
  constructor(file) { this.file = file; this.data = { devices: [], enginePath: '' }; this.write = Promise.resolve(); }
  async load() {
    try {
      const value = JSON.parse(await fs.readFile(this.file, 'utf8'));
      this.data.devices = Array.isArray(value.devices) ? value.devices.map(cleanDevice).slice(0, 50) : [];
      this.data.enginePath = typeof value.enginePath === 'string' ? value.enginePath : '';
    } catch (error) { if (error.code !== 'ENOENT') this.loadError = 'Saved settings could not be read. No connection was started.'; }
    return this.data;
  }
  async save() {
    if (this.loadError) throw new Error('Saved settings are unreadable. Back up and repair devices.json before saving.');
    const snapshot = JSON.stringify(this.data, null, 2);
    this.write = this.write.catch(() => {}).then(async () => {
      await fs.mkdir(path.dirname(this.file), { recursive: true });
      const temporary = this.file + '.tmp';
      await fs.writeFile(temporary, snapshot, { mode: 0o600 });
      await fs.rename(temporary, this.file);
    });
    return this.write;
  }
  async put(value) {
    const device = cleanDevice(value);
    const existing = this.data.devices.findIndex(d => d.id === device.id);
    if (existing >= 0) this.data.devices[existing] = device;
    else {
      if (this.data.devices.length >= 50) throw new Error('Up to 50 computers can be saved.');
      this.data.devices.push(device);
    }
    await this.save(); return device;
  }
  async remove(id) { this.data.devices = this.data.devices.filter(d => d.id !== id); await this.save(); }
}
class EngineLauncher extends EventEmitter {
  constructor(options = {}) {
    super(); this.spawn = options.spawn || spawn; this.delay = options.delay ?? 1200;
    this.verify = options.verify || verifyEngine; this.workingDirectory = options.workingDirectory;
    this.state = { phase: 'idle', message: 'Choose a saved computer or add one to get started.' };
    this.timer = null; this.generation = 0;
  }
  update(state) { this.state = state; this.emit('state', { ...state }); return { ...state }; }
  prepare(device, action, consent, enginePath) {
    if (this.state.phase !== 'idle' && this.state.phase !== 'error') throw new Error('Finish or cancel the current handoff first.');
    if (consent !== true) throw new Error('Confirm that you have permission to connect.');
    const args = launchArguments(device, action);
    if (!enginePath || !path.isAbsolute(enginePath)) throw new Error('Choose the verified RustDesk release in Settings first.');
    const generation = ++this.generation;
    this.update({ phase: 'preparing', device: cleanDevice(device), action, message: 'Opening RustDesk shortly. You can still cancel.' });
    this.timer = setTimeout(async () => {
      if (generation !== this.generation) return;
      this.timer = null;
      let child;
      try {
        const identity = await this.verify(enginePath);
        if (generation !== this.generation) return;
        if (!identity.ready || !identity.verified) return this.update({ ...this.state, phase: 'error', engineInvalid: true, message: identity.message || 'Unverified application. Launch blocked.' });
        child = this.spawn(enginePath, args, engineProcessOptions(this.workingDirectory));
      }
      catch {
        if (generation === this.generation) this.update({ ...this.state, phase: 'error', message: 'RustDesk could not be opened. Check the selected application in Settings.' });
        return;
      }
      child.once('error', () => {
        if (generation === this.generation) this.update({ ...this.state, phase: 'error', message: 'RustDesk could not be opened. Check the selected application in Settings.' });
      });
      child.once('spawn', () => {
        if (generation !== this.generation) return;
        this.update({ ...this.state, phase: 'handed-off', message: 'Continue in RustDesk. Verify the peer and choose permissions there. Connection status is not available to this app.' });
      });
      // RustDesk may forward to an existing process and exit immediately. Process
      // exit is NOT session disconnection; never infer authentication or status.
      child.unref?.();
    }, this.delay);
    return { ...this.state };
  }
  cancel() {
    if (this.state.phase !== 'preparing') throw new Error('After handoff, end the session in RustDesk.');
    clearTimeout(this.timer); this.timer = null; ++this.generation;
    return this.update({ phase: 'idle', message: 'Connection handoff canceled. Nothing was launched.' });
  }
  reset(confirmed) {
    if (this.state.phase === 'preparing') return this.cancel();
    if (this.state.phase === 'handed-off' && confirmed !== true) throw new Error('End the session in RustDesk, then confirm it is closed.');
    ++this.generation;
    return this.update({ phase: 'idle', message: 'Ready. No remote session state is tracked by this app.' });
  }
  dispose() { if (this.timer) clearTimeout(this.timer); ++this.generation; }
}
module.exports = { cleanDevice, launchArguments, DeviceStore, EngineLauncher };
