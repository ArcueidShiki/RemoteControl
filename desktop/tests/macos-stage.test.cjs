'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { APP_FILES, DOCUMENTS, stageFromGit, expectedAsarInventory, verifyAsar, treeInventory, verifyZipNames } = require('../scripts/macos-stage.cjs');

async function fixture(callback) {
  const temp = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-stage-test-'));
  const repository = path.join(temp, 'repository'); await fs.mkdir(repository);
  const git = (...args) => execFileSync('git', args, { cwd: repository, encoding: 'utf8' }).trim();
  async function write(name, data) {
    const file = path.join(repository, name); await fs.mkdir(path.dirname(file), { recursive: true });
    await fs.writeFile(file, data);
  }
  try {
    git('init', '-q'); git('config', 'user.name', 'Synthetic packaging test'); git('config', 'user.email', 'fixture@example.invalid');
    const pkg = { name: 'synthetic-fixture', version: '1.0.0', main: 'src/main.cjs', devDependencies: {} };
    for (const name of APP_FILES) await write(`desktop/${name}`, name === 'package.json' ? JSON.stringify(pkg) : `// committed synthetic fixture: ${name}\n`);
    for (const name of Object.keys(DOCUMENTS)) await write(name, `synthetic notice ${name}\n`);
    await write('desktop/package-lock.json', JSON.stringify({ lockfileVersion: 3, packages: { '': pkg } }));
    await write('.gitignore', '*.pfx\noutput/\nnode_modules/\n');
    git('add', '.'); git('commit', '-qm', 'Synthetic allowlisted fixture');
    const head = git('rev-parse', 'HEAD');
    await callback({ temp, repository, head, git, write, pkg });
  } finally { await fs.rm(temp, { recursive: true, force: true }); }
}

test('Git staging excludes ignored sensitive canaries and untracked build files from ASAR and ZIP', () => fixture(async ({ temp, repository, head, git, write }) => {
  // All canaries are generated here; never inspect real user secrets.
  await write('desktop/private.pfx', 'SYNTHETIC-NOT-A-PRIVATE-KEY');
  await write('desktop/src/ignored.pfx', 'SYNTHETIC-NESTED-KEY');
  await write('desktop/output/build-result.bin', 'SYNTHETIC-IGNORED-BUILD');
  await write('desktop/node_modules/untrusted/index.js', 'SYNTHETIC-LOCAL-MODULE');
  assert.equal(git('status', '--porcelain'), '', 'ignored files evade a clean Git status');
  await write('desktop/generated-build.js', 'SYNTHETIC-UNTRACKED-BUILD');
  await write('desktop/src/injected.js', 'SYNTHETIC-UNTRACKED-SOURCE');
  await write('desktop/src/main.cjs', 'SYNTHETIC-WORKTREE-MUTATION');
  await write('desktop/README.md', 'SYNTHETIC-WORKTREE-DOC-MUTATION');
  const stage = path.join(temp, 'stage'), staged = await stageFromGit(repository, head, stage);
  assert.equal(await fs.readFile(path.join(stage, 'src/main.cjs'), 'utf8'), '// committed synthetic fixture: src/main.cjs\n');
  assert.match(staged.documents.get('READ-ME.md').data.toString(), /^synthetic notice/);
  const tree = await treeInventory(stage);
  assert.deepEqual(Object.keys(tree).filter(name => tree[name].type === 'file').sort(), [...APP_FILES].sort());
  const asar = await import('@electron/asar');
  const archive = path.join(temp, 'app.asar'); await asar.createPackage(stage, archive);
  await verifyAsar(archive, expectedAsarInventory(staged.files));
  if (process.platform === 'darwin') {
    const zip = path.join(temp, 'stage.zip');
    execFileSync('/usr/bin/ditto', ['-c', '-k', '--norsrc', '--noextattr', '--noqtn', '--keepParent', stage, zip]);
    const names = execFileSync('/usr/bin/unzip', ['-Z1', zip], { encoding: 'utf8' }).trim().split('\n');
    verifyZipNames(names, 'stage', tree);
    const extracted = path.join(temp, 'extracted'); await fs.mkdir(extracted);
    execFileSync('/usr/bin/ditto', ['-x', '-k', '--norsrc', '--noextattr', '--noqtn', zip, extracted]);
    assert.deepEqual(await treeInventory(path.join(extracted, 'stage')), tree);
  }
}));

test('full ASAR inventory rejects unexpected root files, altered source and symlinks', () => fixture(async ({ temp, repository, head }) => {
  const stage = path.join(temp, 'stage'), staged = await stageFromGit(repository, head, stage);
  const expected = expectedAsarInventory(staged.files), asar = await import('@electron/asar');
  await fs.writeFile(path.join(stage, 'private.pfx'), 'SYNTHETIC-EXTRA');
  await asar.createPackage(stage, path.join(temp, 'extra.asar'));
  await assert.rejects(verifyAsar(path.join(temp, 'extra.asar'), expected), /inventory differs/);
  await fs.rm(path.join(stage, 'private.pfx'));
  await fs.writeFile(path.join(stage, 'src/main.cjs'), 'changed');
  await asar.createPackage(stage, path.join(temp, 'changed.asar'));
  await assert.rejects(verifyAsar(path.join(temp, 'changed.asar'), expected), /inventory differs/);
  if (process.platform !== 'win32') {
    await fs.symlink('package.json', path.join(stage, 'linked.json'));
    await asar.createPackage(stage, path.join(temp, 'linked.asar'));
    await assert.rejects(verifyAsar(path.join(temp, 'linked.asar'), expected), /linked\/unpacked/);
  }
}));

test('staging fails closed for tracked symlinks and newly introduced runtime dependencies', () => fixture(async ({ temp, repository, git, write, pkg }) => {
  pkg.dependencies = { 'synthetic-runtime-dependency': '1.2.3' };
  await write('desktop/package.json', JSON.stringify(pkg));
  await write('desktop/package-lock.json', JSON.stringify({ lockfileVersion: 3, packages: { '': pkg } }));
  git('add', '.'); git('commit', '-qm', 'Synthetic production dependency');
  await assert.rejects(stageFromGit(repository, git('rev-parse', 'HEAD'), path.join(temp, 'dependency-stage')), /production dependency staging/);
  if (process.platform !== 'win32') {
    await fs.rm(path.join(repository, 'desktop/src/main.cjs'));
    await fs.symlink('core.cjs', path.join(repository, 'desktop/src/main.cjs'));
    git('add', '.'); git('commit', '-qm', 'Synthetic source symlink');
    await assert.rejects(stageFromGit(repository, git('rev-parse', 'HEAD'), path.join(temp, 'link-stage')), /regular tracked Git blob/);
  }
}));

test('ZIP inventory rejects extra/duplicate entries and bundle links cannot escape the root', async () => {
  const expected = { 'a.txt': { type: 'file' } };
  assert.throws(() => verifyZipNames(['root/', 'root/a.txt', 'root/private.pfx'], 'root', expected), /ZIP entry inventory/);
  assert.throws(() => verifyZipNames(['root/', 'root/a.txt', 'root/a.txt'], 'root', expected), /ZIP entry inventory/);
  if (process.platform === 'win32') return;
  const temp = await fs.mkdtemp(path.join(os.tmpdir(), 'remotecontrol-link-test-'));
  try {
    await fs.symlink('../outside', path.join(temp, 'escape'));
    await assert.rejects(treeInventory(temp), /escapes its root/);
  } finally { await fs.rm(temp, { recursive: true, force: true }); }
});
