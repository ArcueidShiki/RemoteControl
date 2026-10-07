'use strict';
const fs = require('node:fs/promises');
const path = require('node:path');
const { createHash } = require('node:crypto');

// Reviewed upstream release assets, not user-editable settings. Updating this
// list requires reviewing the official release and changing the application.
// https://github.com/rustdesk/rustdesk/releases/tag/1.5.0
const RELEASES = Object.freeze({
  'win32-x64': Object.freeze({ version: '1.5.0', size: 25887600,
    sha256: '8555777215510d83d2d61c9dc984e4fcc838bd7e79f9d18a42585431f5e8bb47',
    names: Object.freeze(['rustdesk.exe', 'rustdesk-1.5.0-x86_64.exe']) })
});

async function verifyEngine(executable, options = {}) {
  const platform = options.platform || process.platform, arch = options.arch || process.arch;
  const release = RELEASES[platform + '-' + arch];
  const failure = message => ({ ready: false, verified: false, path: executable || '', message });
  if (!executable) return failure('Choose the verified RustDesk 1.5.0 Windows x64 release.');
  if (!release) return failure('Unverified: engine launch on this platform needs a reviewed release identity.');
  if (!path.isAbsolute(executable)) return failure('Unverified: select an absolute application path.');
  // RustDesk interprets some custom executable names as configuration. Only
  // plain upstream names are accepted, in addition to checking the actual bytes.
  if (!release.names.includes(path.basename(executable).toLowerCase()))
    return failure('Unverified: use rustdesk.exe or rustdesk-1.5.0-x86_64.exe from the reviewed release.');
  let file;
  try {
    file = await fs.open(executable, 'r');
    const before = await file.stat({ bigint: true });
    if (!before.isFile() || before.size !== BigInt(release.size)) return failure('Unverified: this file does not match the reviewed RustDesk release.');
    const hash = createHash('sha256');
    const buffer = Buffer.alloc(1024 * 1024);
    let offset = 0;
    while (offset < release.size) {
      const { bytesRead } = await file.read(buffer, 0, Math.min(buffer.length, release.size - offset), offset);
      if (!bytesRead) return failure('Unverified: application changed while being checked. Select it again.');
      hash.update(buffer.subarray(0, bytesRead)); offset += bytesRead;
    }
    const after = await file.stat({ bigint: true }), current = await fs.stat(executable, { bigint: true });
    for (const stat of [after, current]) {
      // Windows libuv reports dev=0 for stat(path), but a volume ID for fstat.
      if ((process.platform !== 'win32' && stat.dev !== before.dev) || stat.ino !== before.ino || stat.size !== before.size || stat.mtimeNs !== before.mtimeNs || stat.ctimeNs !== before.ctimeNs)
        return failure('Unverified: application changed while being checked. Select it again.');
    }
    if (hash.digest('hex') !== release.sha256) return failure('Unverified: application digest does not match the reviewed RustDesk release.');
    return { ready: true, verified: true, path: executable, version: release.version, sha256: release.sha256,
      message: 'Verified RustDesk ' + release.version + ' Windows x64 release (SHA-256). Checked again before each launch.' };
  } catch { return failure('Unverified: application cannot be read. Choose the reviewed RustDesk release again.'); }
  finally { await file?.close(); }
}

function engineEnvironment(source = process.env, platform = process.platform) {
  const env = {};
  const sourceMap = Object.fromEntries(Object.entries(source).map(([key, value]) => [platform === 'win32' ? key.toLowerCase() : key, value]));
  const keys = platform === 'win32' ? ['SystemRoot', 'WINDIR', 'USERPROFILE', 'APPDATA', 'LOCALAPPDATA', 'TEMP', 'TMP'] : ['HOME', 'TMPDIR'];
  for (const key of keys) {
    const value = sourceMap[platform === 'win32' ? key.toLowerCase() : key];
    if (typeof value === 'string' && value && !/[\0\r\n]/.test(value) && (platform === 'win32' ? path.win32 : path.posix).isAbsolute(value)) env[key] = value;
  }
  if (platform === 'win32') {
    if (!env.SystemRoot) throw new Error('Windows system directory is unavailable.');
    env.PATH = path.win32.join(env.SystemRoot, 'System32') + ';' + env.SystemRoot;
  } else env.PATH = '/usr/bin:/bin:/usr/sbin:/sbin';
  return env;
}

function engineProcessOptions(workingDirectory, overrides = {}) {
  if (!workingDirectory || !path.isAbsolute(workingDirectory)) throw new Error('An absolute application-owned working directory is required.');
  return { windowsHide: false, stdio: 'ignore', detached: true,
    ...overrides, shell: false, cwd: workingDirectory, env: engineEnvironment() };
}
module.exports = { RELEASES, verifyEngine, engineEnvironment, engineProcessOptions };
