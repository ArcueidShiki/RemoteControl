'use strict';
const fs = require('node:fs/promises');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { createHash } = require('node:crypto');
const assert = require('node:assert/strict');

// An explicit distribution contract, not a glob over a developer checkout.
const APP_FILES = Object.freeze(['package.json', 'src/app.js', 'src/core.cjs',
  'src/engine.cjs', 'src/index.html', 'src/main.cjs', 'src/network.cjs', 'src/preload.cjs', 'src/style.css']);
const DOCUMENTS = Object.freeze({ 'LICENSE': 'PROJECT-LICENSE.txt',
  'desktop/README.md': 'READ-ME.md', 'desktop/docs/MACOS-HANDOFF.md': 'MACOS-HANDOFF.md' });
const sha256 = data => createHash('sha256').update(data).digest('hex');

function gitBlob(repository, head, name) {
  if (!/^[a-f0-9]{40}$/.test(head)) throw new Error('A full immutable Git revision is required.');
  const entry = execFileSync('git', ['ls-tree', '-z', head, '--', name], { cwd: repository });
  const match = /^(100644|100755) blob ([a-f0-9]{40})\t([^\0]+)\0$/.exec(entry.toString('utf8'));
  if (!match || match[3] !== name) throw new Error(`Allowlisted input must be a regular tracked Git blob: ${name}`);
  return { data: execFileSync('git', ['cat-file', 'blob', match[2]], { cwd: repository, maxBuffer: 16 * 1024 * 1024 }),
    mode: parseInt(match[1].slice(-3), 8), blob: match[2] };
}

async function stageFromGit(repository, head, destination) {
  // Refuse to overlay a previous staging tree.
  await fs.mkdir(destination);
  const files = new Map();
  for (const name of APP_FILES) {
    const source = gitBlob(repository, head, `desktop/${name}`);
    await fs.mkdir(path.dirname(path.join(destination, name)), { recursive: true });
    await fs.writeFile(path.join(destination, name), source.data, { mode: source.mode, flag: 'wx' });
    files.set(name, source);
  }
  const pkg = JSON.parse(files.get('package.json').data);
  const lock = gitBlob(repository, head, 'desktop/package-lock.json');
  const lockData = JSON.parse(lock.data);
  if (lockData.lockfileVersion !== 3 || !lockData.packages?.['']) throw new Error('Review the new lockfile format before packaging.');
  const lockRoot = lockData.packages[''];
  for (const key of ['name', 'version', 'dependencies', 'optionalDependencies', 'devDependencies'])
    assert.deepEqual(lockRoot[key], pkg[key], `package.json / package-lock.json mismatch: ${key}`);
  // The current application uses only Node builtins and Electron's runtime API.
  // Never silently discard a newly introduced runtime dependency. Supporting one
  // requires a reviewed production allowlist and a clean lockfile install.
  for (const key of ['dependencies', 'optionalDependencies', 'peerDependencies', 'bundledDependencies', 'bundleDependencies'])
    if (pkg[key] && Object.keys(pkg[key]).length) throw new Error(`Review production dependency staging before adding ${key}.`);
  if (Object.entries(lockData.packages).some(([name, item]) => name && !item.dev))
    throw new Error('Lockfile contains unstaged production dependencies.');
  if (!files.has(pkg.main)) throw new Error('Application entry point is outside the explicit allowlist.');
  // Packager normalizes package.json. Generate the same minimal runtime metadata
  // here so every output byte, including package.json, has an explicit expectation.
  const runtimePackage = Object.fromEntries(['name', 'version', 'description', 'main']
    .filter(key => pkg[key] !== undefined).map(key => [key, pkg[key]]));
  const metadata = Buffer.from(JSON.stringify(runtimePackage, null, 2) + '\n');
  await fs.writeFile(path.join(destination, 'package.json'), metadata);
  files.set('package.json', { ...files.get('package.json'), data: metadata });
  const documents = new Map(Object.entries(DOCUMENTS).map(([source, target]) => [target, gitBlob(repository, head, source)]));
  return { pkg, files, documents, lockHash: sha256(lock.data) };
}

function expectedAsarInventory(files) {
  const inventory = new Map();
  for (const [name, { data, mode }] of files) {
    let parent = path.posix.dirname(name);
    while (parent !== '.') { inventory.set(parent, { type: 'directory' }); parent = path.posix.dirname(parent); }
    inventory.set(name, { type: 'file', bytes: data.length, sha256: sha256(data), executable: Boolean(mode & 0o111) });
  }
  return Object.fromEntries([...inventory].sort(([a], [b]) => a.localeCompare(b)));
}

async function verifyAsar(archive, expected) {
  const asar = await import('@electron/asar');
  const inventory = {};
  for (const entry of asar.listPackage(archive)) {
    const name = entry.replace(/^[/\\]/, '').replaceAll('\\', '/');
    const stat = asar.statFile(archive, name, false);
    if (stat.link || stat.unpacked) throw new Error(`Unexpected linked/unpacked ASAR entry: ${name}`);
    inventory[name] = stat.files ? { type: 'directory' } : {
      type: 'file', bytes: stat.size, sha256: sha256(asar.extractFile(archive, name)), executable: Boolean(stat.executable) };
  }
  assert.deepEqual(inventory, expected, 'Complete ASAR inventory differs from the committed allowlist.');
  return inventory;
}

async function treeInventory(directory) {
  const inventory = {};
  async function walk(relative) {
    for (const name of (await fs.readdir(path.join(directory, relative))).sort()) {
      const child = relative ? `${relative}/${name}` : name;
      const file = path.join(directory, child), stat = await fs.lstat(file);
      if (stat.isSymbolicLink()) {
        const target = await fs.readlink(file);
        const resolved = path.resolve(path.dirname(file), target);
        if (path.isAbsolute(target) || !resolved.startsWith(path.resolve(directory) + path.sep))
          throw new Error(`Bundle link escapes its root: ${child}`);
        inventory[child] = { type: 'symlink', target };
      } else if (stat.isDirectory()) {
        inventory[child] = { type: 'directory' }; await walk(child);
      } else if (stat.isFile()) {
        const hash = createHash('sha256'), handle = await fs.open(file, 'r');
        try { for await (const chunk of handle.createReadStream()) hash.update(chunk); }
        finally { await handle.close(); }
        inventory[child] = { type: 'file', bytes: stat.size, sha256: hash.digest('hex'), executable: Boolean(stat.mode & 0o111) };
      } else throw new Error(`Unsupported bundle entry: ${child}`);
    }
  }
  await walk(''); return inventory;
}

function verifyZipNames(names, rootName, inventory) {
  const expected = [rootName + '/', ...Object.entries(inventory)
    .map(([name, entry]) => `${rootName}/${name}${entry.type === 'directory' ? '/' : ''}`)].sort();
  assert.deepEqual(names.slice().sort(), expected, 'Complete ZIP entry inventory differs from the clean package.');
}

module.exports = { APP_FILES, DOCUMENTS, gitBlob, stageFromGit, expectedAsarInventory,
  verifyAsar, treeInventory, verifyZipNames, sha256 };
