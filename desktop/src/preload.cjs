'use strict';
const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('remoteControl', {
  snapshot: () => ipcRenderer.invoke('workspace:snapshot'),
  saveDevice: value => ipcRenderer.invoke('workspace:save-device', value),
  removeDevice: id => ipcRenderer.invoke('workspace:remove-device', id),
  chooseEngine: () => ipcRenderer.invoke('workspace:choose-engine'),
  prepare: value => ipcRenderer.invoke('workspace:prepare', value),
  cancel: () => ipcRenderer.invoke('workspace:cancel'),
  reset: confirmed => ipcRenderer.invoke('workspace:reset', confirmed),
  documentation: () => ipcRenderer.invoke('workspace:documentation'),
  onSession: callback => {
    const listener = (_event, state) => callback(state);
    ipcRenderer.on('workspace:session', listener);
    return () => ipcRenderer.removeListener('workspace:session', listener);
  }
});
