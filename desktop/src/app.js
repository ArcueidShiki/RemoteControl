'use strict';
const $ = id => document.getElementById(id);
const api = window.remoteControl;
let model, selectedId = null;
async function call(promise) { const result = await promise; if (!result.ok) throw new Error(result.error); return result.value; }
function alertMessage(message = '') { $('alert').textContent = message; $('alert').hidden = !message; }
async function action(fn) { try { alertMessage(); await fn(); } catch (error) { alertMessage(error.message); } }
function page(name) {
  for (const item of document.querySelectorAll('.page')) item.hidden = item.id !== name + '-page';
  for (const button of document.querySelectorAll('.nav')) button.classList.toggle('active', button.dataset.page === name);
  $('page-name').textContent = { devices: 'Computers', settings: 'Settings', lab: 'Connection lab' }[name];
  if (name !== 'lab') stopLab();
  alertMessage();
}
for (const button of document.querySelectorAll('.nav')) button.addEventListener('click', () => page(button.dataset.page));
document.querySelector('.brand').addEventListener('click', event => { event.preventDefault(); page('devices'); });
function openDevice() { $('device-form').reset(); $('device-error').textContent = ''; $('device-dialog').showModal(); $('device-name').focus(); }
for (const id of ['add-device', 'empty-add']) $(id).addEventListener('click', openDevice);
for (const id of ['close-dialog', 'cancel-device']) $(id).addEventListener('click', () => $('device-dialog').close());
$('device-form').addEventListener('submit', async event => {
  event.preventDefault();
  try {
    model = await call(api.saveDevice({ name: $('device-name').value, peerId: $('device-id').value, peerIp: $('device-ip').value }));
    selectedId = $('device-id').value.replace(/\s/g, ''); $('consent').checked = false;
    $('device-dialog').close(); render();
  } catch (error) { $('device-error').textContent = error.message; }
});
$('search').addEventListener('input', renderDevices);
function renderDevices() {
  const list = $('device-list'); list.replaceChildren();
  const search = $('search').value.toLowerCase();
  const items = model.devices.filter(d => (d.name + d.peerId + d.peerIp).toLowerCase().includes(search));
  $('device-count').textContent = model.devices.length;
  $('empty-state').hidden = model.devices.length > 0;
  for (const device of items) {
    const wrapper = document.createElement('div');
    const button = document.createElement('button');
    button.className = 'device-card' + (device.id === selectedId ? ' selected' : '');
    button.setAttribute('aria-label', 'Select ' + device.name);
    button.disabled = ['preparing', 'handed-off'].includes(model.session.phase);
    const icon = document.createElement('span'); icon.className = 'mini-icon'; icon.textContent = '▣';
    const content = document.createElement('div'); content.className = 'device-meta';
    const title = document.createElement('h3'); title.textContent = device.name;
    const id = document.createElement('p'); id.textContent = 'RustDesk · ' + device.peerId;
    const ip = document.createElement('small'); ip.textContent = device.peerIp || 'Peer IP not supplied';
    content.append(title, id, ip);
    const arrow = document.createElement('span'); arrow.className = 'arrow'; arrow.textContent = '↗';
    button.append(icon, content, arrow);
    button.addEventListener('click', () => { selectedId = device.id; $('consent').checked = false; render(); });
    const remove = document.createElement('button'); remove.className = 'remove'; remove.textContent = 'Remove from workspace';
    remove.setAttribute('aria-label', 'Remove ' + device.name); remove.disabled = button.disabled;
    remove.addEventListener('click', () => action(async () => { model = await call(api.removeDevice(device.id)); if (selectedId === device.id) selectedId = null; render(); }));
    wrapper.append(button, remove); list.append(wrapper);
  }
  if (!items.length && model.devices.length) { const text = document.createElement('p'); text.textContent = 'No matching computers.'; list.append(text); }
}
function render() {
  if (selectedId && !model.devices.some(d => d.id === selectedId)) selectedId = null;
  const device = model.devices.find(d => d.id === selectedId);
  $('local-name').textContent = model.local.name;
  $('local-addresses').textContent = model.local.addresses.length ? 'Local IPv4 · ' + model.local.addresses.join(' · ') : 'Loopback · 127.0.0.1';
  $('setup-banner').hidden = model.engine.ready;
  $('engine-path').textContent = model.engine.path || 'No application selected';
  $('engine-verification').textContent = model.engine.message || '';
  $('engine-summary').textContent = model.engine.ready ? 'RustDesk · verified' : model.engine.path ? 'Unverified · blocked' : 'Setup needed';
  $('session-name').textContent = device?.name || 'Choose a computer';
  $('peer-id').textContent = device?.peerId || '—';
  $('peer-ip').textContent = device?.peerIp ? device.peerIp + ' (label)' : 'Not supplied';
  const state = model.session;
  $('session-badge').textContent = { idle: 'Ready', preparing: 'Opening', 'handed-off': 'In RustDesk', error: 'Needs attention' }[state.phase];
  $('session-description').textContent = state.message;
  const pending = state.phase === 'preparing', handedOff = state.phase === 'handed-off';
  $('connection-actions').hidden = pending || handedOff;
  $('handoff-actions').hidden = !pending && !handedOff;
  $('cancel-handoff').hidden = !pending;
  $('reset-handoff').hidden = !handedOff;
  $('connect').disabled = !device || !model.engine.ready || !$('consent').checked || pending || handedOff;
  $('add-device').disabled = pending || handedOff;
  renderDevices();
}
$('consent').addEventListener('change', render);
$('connect').addEventListener('click', () => action(async () => {
  model.session = await call(api.prepare({ id: selectedId, action: $('action').value, consent: $('consent').checked })); render();
}));
$('cancel-handoff').addEventListener('click', () => action(async () => { model.session = await call(api.cancel()); $('consent').checked = false; render(); }));
$('reset-handoff').addEventListener('click', () => action(async () => { model.session = await call(api.reset(true)); $('consent').checked = false; render(); }));
$('setup-engine').addEventListener('click', () => page('settings'));
$('choose-engine').addEventListener('click', () => action(async () => { model = await call(api.chooseEngine()); render(); }));
$('engine-docs').addEventListener('click', () => action(() => call(api.documentation())));
api.onSession(state => { if (model) { model.session = state; render(); if (state.engineInvalid) action(async () => { model = await call(api.snapshot()); render(); }); } });
action(async () => { model = await call(api.snapshot()); render(); if (model.loadError) alertMessage(model.loadError); });

// A local-only lab, independent of the external RustDesk engine. Generated
// canvas pixels travel through two actual WebRTC peers; no desktop capture.
let lab = null, labGeneration = 0;
function stopLab() {
  ++labGeneration;
  if (lab) {
    clearInterval(lab.drawTimer); clearInterval(lab.statsTimer);
    lab.stream?.getTracks().forEach(track => track.stop());
    lab.send?.close(); lab.receive?.close(); lab = null;
  }
  $('lab-video').srcObject = null; $('lab-placeholder').hidden = false;
  $('lab-start').disabled = false; $('lab-stop').disabled = true; $('lab-resolution').disabled = false;
  $('lab-ping').disabled = true; $('lab-keyboard').disabled = true;
  $('lab-state').textContent = 'No test running';
  for (const id of ['metric-resolution', 'metric-fps', 'metric-rtt', 'metric-codec']) $(id).textContent = '—';
  $('lab-keyboard').value = ''; $('lab-echo').textContent = 'No input sent';
}
async function gathered(peer) {
  if (peer.iceGatheringState === 'complete') return;
  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => { peer.removeEventListener('icegatheringstatechange', check); reject(new Error('Local WebRTC setup timed out. Try again.')); }, 6000);
    const check = () => { if (peer.iceGatheringState === 'complete') { clearTimeout(timeout); peer.removeEventListener('icegatheringstatechange', check); resolve(); } };
    peer.addEventListener('icegatheringstatechange', check);
  });
}
function echoInput(kind) {
  if (!lab || lab.channel?.readyState !== 'open') return;
  const id = ++lab.inputCount;
  lab.sent.set(id, performance.now());
  lab.channel.send(JSON.stringify({ id, kind })); // No character or clipboard content.
}
$('lab-ping').addEventListener('click', () => echoInput('pointer'));
$('lab-keyboard').addEventListener('keydown', event => {
  if (event.key === 'Tab') return;
  event.preventDefault(); echoInput('key');
});
$('lab-stop').addEventListener('click', stopLab);
$('lab-start').addEventListener('click', () => action(async () => {
  stopLab(); const generation = labGeneration;
  $('lab-start').disabled = true; $('lab-stop').disabled = false; $('lab-resolution').disabled = true;
  $('lab-state').textContent = 'Negotiating local peers…';
  const height = Number($('lab-resolution').value), width = height * 16 / 9;
  const source = $('lab-source'); source.width = width; source.height = height;
  const context = source.getContext('2d', { alpha: false });
  const current = lab = { send: new RTCPeerConnection({ iceServers: [] }), receive: new RTCPeerConnection({ iceServers: [] }), inputCount: 0, sent: new Map(), last: null };
  const check = () => { if (generation !== labGeneration) throw new Error('Local test canceled.'); };
  let frame = 0;
  const draw = () => {
    ++frame;
    context.fillStyle = '#14213c'; context.fillRect(0, 0, width, height);
    context.strokeStyle = '#253958'; context.lineWidth = 1;
    for (let x = 0; x < width; x += 80) { context.beginPath(); context.moveTo(x, 0); context.lineTo(x, height); context.stroke(); }
    for (let y = 0; y < height; y += 80) { context.beginPath(); context.moveTo(0, y); context.lineTo(width, y); context.stroke(); }
    context.fillStyle = '#b9cbf5'; context.font = Math.round(height / 25) + 'px sans-serif';
    context.fillText('LOCAL WEBRTC · SYNTHETIC SOURCE', width * .08, height * .17);
    context.fillStyle = '#fff'; context.font = Math.round(height / 10) + 'px sans-serif';
    context.fillText(width + ' × ' + height, width * .08, height * .38);
    context.font = Math.round(height / 32) + 'px sans-serif'; context.fillStyle = '#89a1ca';
    context.fillText('No screen capture. No remote computer.', width * .08, height * .48);
    context.fillStyle = '#77a1ff'; context.beginPath();
    context.arc(width * (.12 + .75 * ((frame % 180) / 180)), height * .73, height * .047, 0, Math.PI * 2); context.fill();
    context.font = Math.round(height / 38) + 'px monospace'; context.fillStyle = '#acc2ea';
    context.fillText('Source frame ' + frame, width * .08, height * .91);
  };
  draw(); current.drawTimer = setInterval(draw, 1000 / 30);
  current.stream = source.captureStream(30);
  for (const track of current.stream.getTracks()) { track.contentHint = 'detail'; current.send.addTrack(track, current.stream); }
  current.channel = current.send.createDataChannel('input-echo', { ordered: true });
  current.channel.onopen = () => { if (lab === current) { $('lab-ping').disabled = false; $('lab-keyboard').disabled = false; } };
  current.channel.onmessage = event => {
    if (lab !== current) return;
    const data = JSON.parse(event.data); const start = current.sent.get(data.id);
    if (start !== undefined) { $('metric-rtt').textContent = (performance.now() - start).toFixed(1) + ' ms'; current.sent.delete(data.id); $('lab-echo').textContent = data.kind + ' event acknowledged · #' + data.id; }
  };
  current.receive.ondatachannel = event => { event.channel.onmessage = message => { if (event.channel.readyState === 'open') event.channel.send(message.data); }; };
  current.receive.ontrack = event => { if (lab === current) { $('lab-video').srcObject = event.streams[0]; $('lab-placeholder').hidden = true; } };
  try {
    await current.send.setLocalDescription(await current.send.createOffer()); await gathered(current.send); check();
    await current.receive.setRemoteDescription(current.send.localDescription);
    await current.receive.setLocalDescription(await current.receive.createAnswer()); await gathered(current.receive); check();
    await current.send.setRemoteDescription(current.receive.localDescription); check();
    current.statsTimer = setInterval(async () => {
      if (lab !== current || current.readingStats) return;
      current.readingStats = true;
      try {
        const stats = await current.receive.getStats();
        if (lab !== current) return;
        for (const item of stats.values()) {
          if (item.type === 'transport' && item.dtlsState === 'connected') $('lab-state').textContent = 'Local peers · DTLS connected';
          if (item.type === 'inbound-rtp' && item.kind === 'video') {
            $('metric-resolution').textContent = (item.frameWidth || $('lab-video').videoWidth) + ' × ' + (item.frameHeight || $('lab-video').videoHeight);
            const previous = current.last;
            if (previous && item.timestamp > previous.timestamp) $('metric-fps').textContent = ((item.framesDecoded - previous.framesDecoded) * 1000 / (item.timestamp - previous.timestamp)).toFixed(1);
            current.last = { timestamp: item.timestamp, framesDecoded: item.framesDecoded };
            const codec = stats.get(item.codecId); $('metric-codec').textContent = codec?.mimeType?.replace('video/', '') || 'Negotiating';
          }
        }
      } catch (error) {
        if (lab === current) $('lab-state').textContent = 'Metrics unavailable. Stop and retry the local test.';
      } finally { current.readingStats = false; }
    }, 1000);
  } catch (error) {
    if (generation !== labGeneration) return;
    stopLab(); throw error;
  }
}));
window.addEventListener('beforeunload', stopLab);
