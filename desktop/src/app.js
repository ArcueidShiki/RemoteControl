'use strict';
const $ = id => document.getElementById(id);
const api = window.remoteControl;
let model, selectedId = null, savingDevice = false, wizardStep = 0, wizardBusy = false;
const busySession = () => model && ['preparing', 'handed-off'].includes(model.session.phase);
const messages = {
  'Enter a RustDesk ID with 6-12 digits. Direct IP connections are not supported by this adapter.': '请输入 6–12 位数字 RustDesk ID；这里不能用 IP 直接连接。',
  'Use a computer name between 1 and 60 characters.': '请填写 1–60 个字符的电脑名称。',
  'Peer IP must be a valid IPv4 or IPv6 address, or left blank.': 'IP 备注应为有效的 IPv4 / IPv6 地址，也可以留空。',
  'Saved settings could not be read. No connection was started.': '已保存的设置无法读取。没有发起连接；请先备份并修复设置文件。',
  'Saved settings are unreadable. Back up and repair devices.json before saving.': '设置文件无法读取。请先备份并修复 devices.json，避免覆盖原有数据。',
  'Finish the current handoff first.': '请先取消打开，或在 RustDesk 中结束当前会话。',
  'Choose the verified RustDesk release in Settings first.': '请先在设置中选择经校验的 RustDesk 程序。',
  'RustDesk could not be opened. Check the selected application in Settings.': '无法打开 RustDesk。请在设置中检查所选程序，然后重试。',
  'Up to 50 computers can be saved.': '最多保存 50 台电脑，请先移除不再使用的记录。'
};
const readable = message => messages[message] || message;
async function call(promise) { const result = await promise; if (!result.ok) throw new Error(readable(result.error)); return result.value; }
function alertMessage(message = '') { $('alert').textContent = readable(message); $('alert').hidden = !message; }
async function action(fn) { try { alertMessage(); await fn(); } catch (error) { alertMessage(error.message); } }
function page(name) {
  for (const item of document.querySelectorAll('.page')) item.hidden = item.id !== name + '-page';
  for (const button of document.querySelectorAll('.nav')) button.classList.toggle('active', button.dataset.page === name);
  $('page-name').textContent = { devices: '我的电脑', settings: '设置与帮助', lab: '本机实验' }[name];
  if (name !== 'lab') stopLab();
  alertMessage();
}
for (const button of document.querySelectorAll('[data-page]')) button.addEventListener('click', () => page(button.dataset.page));
document.querySelector('.brand').addEventListener('click', event => { event.preventDefault(); page('devices'); });
function openDevice() {
  if (busySession()) return;
  $('device-form').reset(); $('device-error').textContent = ''; $('device-dialog').showModal(); $('device-name').focus();
}
$('add-device').addEventListener('click', openDevice);
for (const id of ['close-dialog', 'cancel-device']) $(id).addEventListener('click', () => { if (!savingDevice) $('device-dialog').close(); });
$('device-dialog').addEventListener('cancel', event => { if (savingDevice) event.preventDefault(); });
$('device-form').addEventListener('submit', async event => {
  event.preventDefault();
  if (savingDevice) return;
  savingDevice = true;
  for (const id of ['save-device', 'close-dialog', 'cancel-device']) $(id).disabled = true;
  const value = { name: $('device-name').value, peerId: $('device-id').value, peerIp: $('device-ip').value };
  try {
    model = await call(api.saveDevice(value));
    selectedId = value.peerId.replace(/\s/g, ''); $('consent').checked = false;
    $('device-dialog').close(); render();
  } catch (error) { $('device-error').textContent = error.message; }
  finally { savingDevice = false; for (const id of ['save-device', 'close-dialog', 'cancel-device']) $(id).disabled = false; }
});
$('search').addEventListener('input', renderDevices);
function renderDevices() {
  const list = $('device-list'); list.replaceChildren();
  const search = $('search').value.toLowerCase();
  const items = model.devices.filter(d => (d.name + d.peerId + d.peerIp).toLowerCase().includes(search));
  $('device-count').textContent = model.devices.length;
  $('empty-state').hidden = model.devices.length > 0;
  $('search').hidden = model.devices.length === 0;
  for (const device of items) {
    const wrapper = document.createElement('div');
    const button = document.createElement('button');
    button.className = 'device-card' + (device.id === selectedId ? ' selected' : '');
    button.setAttribute('aria-label', '选择 ' + device.name); button.disabled = busySession();
    const icon = document.createElement('span'); icon.className = 'mini-icon'; icon.textContent = '▣'; icon.setAttribute('aria-hidden', 'true');
    const content = document.createElement('div'); content.className = 'device-meta';
    const title = document.createElement('h3'); title.textContent = device.name;
    const id = document.createElement('p'); id.textContent = 'RustDesk ID · ' + device.peerId;
    const ip = document.createElement('small'); ip.textContent = device.peerIp ? device.peerIp + ' · 地址备注' : '未填写地址备注';
    content.append(title, id, ip);
    const arrow = document.createElement('span'); arrow.className = 'arrow'; arrow.textContent = '→';
    button.append(icon, content, arrow);
    button.addEventListener('click', () => { selectedId = device.id; $('consent').checked = false; render(); });
    const remove = document.createElement('button'); remove.className = 'remove'; remove.textContent = '移除记录';
    remove.setAttribute('aria-label', '移除 ' + device.name); remove.disabled = button.disabled;
    remove.addEventListener('click', () => action(async () => { model = await call(api.removeDevice(device.id)); if (selectedId === device.id) selectedId = null; render(); }));
    wrapper.append(button, remove); list.append(wrapper);
  }
  if (!items.length && model.devices.length) { const text = document.createElement('p'); text.textContent = '没有匹配的电脑。'; list.append(text); }
}
function engineLabel() {
  if (model.testMode) return 'RustDesk 测试替身 · 不启动真实程序';
  return model.engine.ready ? 'RustDesk · 文件已校验' : model.engine.path ? '未通过校验 · 已阻止' : '尚未选择程序';
}
function render() {
  if (selectedId && !model.devices.some(d => d.id === selectedId)) selectedId = null;
  const device = model.devices.find(d => d.id === selectedId);
  $('local-name').textContent = model.local.name;
  $('local-addresses').textContent = model.local.addresses.length ? '本机 IPv4 · ' + model.local.addresses.join(' · ') : '未发现非回环 IPv4 地址';
  $('setup-banner').hidden = model.engine.ready;
  $('engine-path').textContent = model.engine.path || '尚未选择程序';
  $('engine-verification').textContent = model.engine.message || '';
  $('engine-summary').textContent = engineLabel();
  $('session-name').textContent = device?.name || '选择一台电脑';
  $('peer-id').textContent = device?.peerId || '—';
  $('peer-ip').textContent = device?.peerIp ? device.peerIp + '（备注）' : '未填写';
  const state = model.session;
  $('session-badge').textContent = { idle: '未连接', preparing: '准备打开', 'handed-off': '状态未知', error: '打开失败' }[state.phase];
  $('session-description').textContent = state.phase === 'preparing' ? '即将打开外部 RustDesk，仍可取消。'
    : state.phase === 'handed-off' ? '已交给 RustDesk。请在其窗口内确认身份和权限；本应用无法获知连接是否成功。'
    : state.phase === 'error' ? readable(state.message)
    : state.message.includes('canceled') ? '已取消，没有打开连接程序。'
    : '选择电脑后，在 RustDesk 中继续。当前没有由本应用确认的远程会话。';
  const pending = state.phase === 'preparing', handedOff = state.phase === 'handed-off';
  $('connection-actions').hidden = pending || handedOff;
  $('handoff-actions').hidden = !pending && !handedOff;
  $('cancel-handoff').hidden = !pending; $('reset-handoff').hidden = !handedOff;
  $('connect').disabled = !device || !model.engine.ready || !$('consent').checked || pending || handedOff;
  for (const id of ['add-device', 'start-guide', 'setup-engine', 'empty-add', 'choose-engine']) $(id).disabled = pending || handedOff;
  renderDevices();
}
$('consent').addEventListener('change', render);
$('connect').addEventListener('click', () => action(async () => {
  model.session = await call(api.prepare({ id: selectedId, action: $('action').value, consent: $('consent').checked })); render();
}));
$('cancel-handoff').addEventListener('click', () => action(async () => { model.session = await call(api.cancel()); $('consent').checked = false; render(); }));
$('reset-handoff').addEventListener('click', () => action(async () => { model.session = await call(api.reset(true)); $('consent').checked = false; render(); }));
$('choose-engine').addEventListener('click', () => action(async () => { model = await call(api.chooseEngine()); render(); }));
$('engine-docs').addEventListener('click', () => action(() => call(api.documentation())));
api.onSession(state => { if (model) { model.session = state; render(); if (state.engineInvalid) action(async () => { model = await call(api.snapshot()); render(); }); } });

function renderWizard() {
  for (const item of document.querySelectorAll('[data-wizard-step]')) item.hidden = Number(item.dataset.wizardStep) !== wizardStep;
  [...document.querySelectorAll('.wizard-steps li')].forEach((item, index) => {
    item.classList.toggle('current', index === wizardStep);
    if (index === wizardStep) item.setAttribute('aria-current', 'step'); else item.removeAttribute('aria-current');
  });
  $('wizard-title').textContent = ['准备连接程序', '了解两台电脑的网络', '添加你的另一台电脑', '核对设备与权限'][wizardStep];
  $('wizard-back').hidden = wizardStep === 0;
  $('wizard-next').textContent = wizardStep === 3 ? '保存并返回主页' : '下一步';
  $('wizard-engine-status').textContent = engineLabel();
  $('wizard-local-name').textContent = model.local.name;
  const container = $('wizard-addresses'); container.replaceChildren();
  const interfaces = model.local.interfaces || model.local.addresses.map(address => ({ address, name: '本机', kind: 'other-address' }));
  for (const entry of interfaces) {
    const row = document.createElement('div'); row.className = 'address-row';
    const address = document.createElement('code'); address.textContent = entry.address;
    const caption = document.createElement('span'); caption.textContent = entry.name + ' · ' + ({
      'tailscale-adapter': 'Tailscale 网卡名称；状态未核实', 'private-address': '私有地址；不代表对端可达', 'other-address': '本机地址；用途未核实'
    }[entry.kind] || '用途未知');
    row.append(address, caption); container.append(row);
  }
  if (!interfaces.length) container.textContent = '未发现非回环地址。可以稍后刷新；当前不会连接。';
  const id = $('wizard-id').value.replace(/\s/g, '');
  const existing = model.devices.some(device => device.id === id);
  $('wizard-peer-summary').textContent = $('wizard-name').value.trim() + ' · ' + id + (existing ? '（将更新已有记录）' : '');
  $('wizard-peer-address').textContent = $('wizard-ip').value.trim() ? '地址备注：' + $('wizard-ip').value.trim() : '未填写地址备注';
}
function wizardLock(locked) {
  wizardBusy = locked;
  for (const element of $('setup-dialog').querySelectorAll('button, input')) element.disabled = locked;
}
function openWizard() {
  if (!model || busySession() || $('setup-dialog').open) return;
  for (const id of ['wizard-name', 'wizard-id', 'wizard-ip']) $(id).value = '';
  wizardStep = 0; $('wizard-error').textContent = ''; renderWizard();
  $('setup-dialog').showModal(); $('wizard-title').focus();
}
function closeWizard() { if (!wizardBusy) $('setup-dialog').close(); }
for (const id of ['start-guide', 'setup-engine', 'empty-add']) $(id).addEventListener('click', openWizard);
for (const id of ['wizard-close', 'wizard-cancel']) $(id).addEventListener('click', closeWizard);
$('setup-dialog').addEventListener('cancel', event => { if (wizardBusy) event.preventDefault(); });
$('wizard-back').addEventListener('click', () => { if (!wizardBusy && wizardStep) { wizardStep--; $('wizard-error').textContent = ''; renderWizard(); $('wizard-title').focus(); } });
for (const [id, method] of [['wizard-engine', 'chooseEngine'], ['wizard-refresh', 'snapshot']]) {
  $(id).addEventListener('click', async () => {
    if (wizardBusy) return;
    wizardLock(true); $('wizard-error').textContent = '';
    try { model = await call(api[method]()); render(); renderWizard(); }
    catch (error) { $('wizard-error').textContent = error.message; }
    finally { wizardLock(false); }
  });
}
$('wizard-next').addEventListener('click', async () => {
  if (wizardBusy) return;
  $('wizard-error').textContent = '';
  const device = { name: $('wizard-name').value, peerId: $('wizard-id').value, peerIp: $('wizard-ip').value };
  wizardLock(true);
  try {
    if (wizardStep === 2) await call(api.validateDevice(device));
    if (wizardStep === 3) {
      model = await call(api.saveDevice(device));
      selectedId = device.peerId.replace(/\s/g, ''); $('consent').checked = false;
      $('search').value = ''; $('setup-dialog').close(); page('devices'); render(); $('session-name').scrollIntoView({ block: 'nearest' });
    } else { wizardStep++; renderWizard(); $('wizard-title').focus(); }
  } catch (error) { $('wizard-error').textContent = error.message; }
  finally { wizardLock(false); }
});
action(async () => { model = await call(api.snapshot()); render(); if (model.loadError) alertMessage(model.loadError); else if (!model.devices.length) openWizard(); });

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
