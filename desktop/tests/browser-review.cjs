'use strict';
// Browser-only review harness. Not packaged. Uses the same device/session model
// and a fake child process; actual WebRTC lab remains unchanged.
const http = require('node:http');
const fs = require('node:fs/promises');
const path = require('node:path');
const { EventEmitter } = require('node:events');
const { DeviceStore, EngineLauncher } = require('../src/core.cjs');
const root = path.resolve(__dirname, '..');
const store = new DeviceStore(path.join(root, '.userdata', 'browser-review', 'devices.json'));
const launcher = new EngineLauncher({ delay: 1200, spawn: () => {
  const child = new EventEmitter(); child.unref = () => {};
  setImmediate(() => child.emit('spawn')); return child;
} });
const snapshot = () => ({ devices: store.data.devices, engine: { ready: true, path: 'TEST ADAPTER — no real engine launched' },
  local: { name: 'Local test computer', addresses: ['127.0.0.1'], platform: 'win32' }, session: launcher.state, testMode: true });
const bridge = [
  'let callback; const invoke=async(method,value)=>{const response=await fetch("/api/"+method,{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(value??null)});return response.json()};',
  'window.remoteControl={snapshot:()=>invoke("snapshot"),saveDevice:v=>invoke("save",v),removeDevice:v=>invoke("remove",v),chooseEngine:()=>invoke("snapshot"),prepare:v=>invoke("prepare",v),cancel:()=>invoke("cancel"),reset:v=>invoke("reset",v),documentation:()=>Promise.resolve({ok:true}),onSession:fn=>{callback=fn;}};',
  'let previous;setInterval(async()=>{const result=await invoke("snapshot");const state=JSON.stringify(result.value.session);if(previous!==state){previous=state;callback?.(result.value.session)}},100);'
].join('\n');
const server = http.createServer(async (request, response) => {
  try {
    const origin = 'http://127.0.0.1:' + server.address().port; const url = new URL(request.url, origin);
    if (request.method === 'POST' && url.pathname.startsWith('/api/')) {
      if (request.headers.origin !== origin) throw new Error('Invalid origin');
      let bytes = ''; for await (const chunk of request) { bytes += chunk; if (bytes.length > 8192) throw new Error('Too large'); }
      const value = JSON.parse(bytes); let result;
      switch (url.pathname) {
        case '/api/snapshot': result = snapshot(); break;
        case '/api/save': await store.put(value); result = snapshot(); break;
        case '/api/remove': await store.remove(value); result = snapshot(); break;
        case '/api/prepare': result = launcher.prepare(store.data.devices.find(d => d.id === value.id), value.action, value.consent, process.execPath); break;
        case '/api/cancel': result = launcher.cancel(); break;
        case '/api/reset': result = launcher.reset(value); break;
        default: throw new Error('Unsupported test action');
      }
      response.setHeader('Content-Type', 'application/json'); return response.end(JSON.stringify({ ok: true, value: result }));
    }
    const allowed = { '/': 'index.html', '/index.html': 'index.html', '/app.js': 'app.js', '/style.css': 'style.css' };
    if (url.pathname === '/test-bridge.js') { response.setHeader('Content-Type', 'application/javascript'); return response.end(bridge); }
    if (!allowed[url.pathname]) { response.statusCode = 404; return response.end(); }
    let content = await fs.readFile(path.join(root, 'src', allowed[url.pathname]), 'utf8');
    if (allowed[url.pathname] === 'index.html') content = content.replace('<script src="app.js">', '<script src="test-bridge.js"></script><script src="app.js">');
    response.setHeader('Content-Type', url.pathname.endsWith('.js') ? 'application/javascript' : url.pathname.endsWith('.css') ? 'text/css' : 'text/html'); response.end(content);
  } catch (error) { response.setHeader('Content-Type', 'application/json'); response.end(JSON.stringify({ ok: false, error: error.message })); }
});
store.load().then(() => server.listen(0, '127.0.0.1', () => console.log('Browser review fixture: http://127.0.0.1:' + server.address().port)));
process.on('SIGINT', () => { launcher.dispose(); server.close(); });
