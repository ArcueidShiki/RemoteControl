'use strict';
const path = require('node:path');
const fs = require('node:fs/promises');
(async () => {
  const { packager } = await import('@electron/packager');
  const platform = process.argv[2], arch = process.argv[3];
  if (!['win32', 'darwin'].includes(platform) || !['x64', 'arm64'].includes(arch)) throw new Error('Choose win32/darwin and x64/arm64.');
  if (platform === 'darwin' && process.platform !== 'darwin') throw new Error('Build and validate the Mac package on an authorized Mac.');
  const root = path.resolve(__dirname, '..');
  const outputs = await packager({ dir: root, out: path.join(root, 'dist'), name: 'RemoteControl', platform, arch,
    electronVersion: require('../package.json').devDependencies.electron, electronZipDir: process.env.ELECTRON_ZIP_DIR || undefined, overwrite: true, asar: true,
    appBundleId: 'org.arcueidshiki.remotecontrol', appCategoryType: 'public.app-category.productivity',
    ignore: [/^\/tests/, /^\/scripts/, /^\/output/, /^\/dist/, /^\/\.userdata/, /^\/node_modules/, /^\/README\.md/],
    win32metadata: { ProductName: 'RemoteControl', FileDescription: 'Cross-platform remote desktop workspace' } });
  for (const output of outputs) {
    await fs.copyFile(path.join(root, '..', 'LICENSE'), path.join(output, 'PROJECT-LICENSE.txt'));
    await fs.copyFile(path.join(root, 'README.md'), path.join(output, 'READ-ME.md'));
    console.log(output);
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
