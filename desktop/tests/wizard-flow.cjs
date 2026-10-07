'use strict';
const path = require('node:path');
module.exports = async function wizardFlow(page, evidence, check) {
  await page.locator('#setup-dialog[open]').waitFor();
  check((await page.locator('#wizard-engine-status').textContent()).includes('测试替身'), 'wizard labels the mocked engine honestly');
  await page.screenshot({ path: path.join(evidence, 'wizard-01-program.png') });
  await page.locator('#wizard-next').click();
  check((await page.locator('.network-status').textContent()).includes('未知'), 'network readiness and session encryption remain unknown');
  await page.locator('#wizard-refresh').click();
  await page.screenshot({ path: path.join(evidence, 'wizard-02-network.png') });
  await page.locator('#wizard-next').click();
  await page.locator('#wizard-name').fill('暂存但取消');
  await page.locator('#wizard-id').fill('123123123');
  await page.locator('#wizard-back').click(); await page.locator('#wizard-next').click();
  check(await page.locator('#wizard-name').inputValue() === '暂存但取消', 'back/forward preserves unsaved wizard fields');
  await page.keyboard.press('Escape');
  check((await page.evaluate(() => window.remoteControl.snapshot())).value.devices.length === 0, 'Escape cancels the wizard without saving');
  await page.locator('#start-guide').click();
  await page.locator('#wizard-next').click(); await page.locator('#wizard-next').click();
  check(await page.locator('#wizard-name').inputValue() === '', 'reopening a canceled wizard starts with an empty draft');
  await page.locator('#wizard-next').click();
  await page.waitForFunction(() => document.getElementById('wizard-error').textContent.includes('名称'));
  await page.locator('#wizard-name').fill('我的 Mac · 向导测试'); await page.locator('#wizard-id').fill('192.168.1.20');
  await page.locator('#wizard-next').click();
  await page.waitForFunction(() => document.getElementById('wizard-error').textContent.includes('6–12'));
  check(true, 'wizard validation rejects a direct IP in the identity field');
  await page.locator('#wizard-id').fill('321 654 987'); await page.locator('#wizard-ip').fill('not-an-ip');
  await page.locator('#wizard-next').click();
  await page.waitForFunction(() => document.getElementById('wizard-error').textContent.includes('IPv4'));
  check(true, 'invalid IP annotation is rejected before the review step');
  await page.locator('#wizard-ip').fill('100.64.1.20');
  await page.screenshot({ path: path.join(evidence, 'wizard-03-peer.png') });
  await page.locator('#wizard-next').click();
  check((await page.evaluate(() => window.remoteControl.snapshot())).value.devices.length === 0, 'reviewing permissions has not saved or launched a session');
  await page.screenshot({ path: path.join(evidence, 'wizard-04-review.png') });
  await page.locator('#wizard-next').evaluate(button => { button.click(); button.click(); });
  await page.waitForFunction(() => !document.getElementById('setup-dialog').open);
  const state = (await page.evaluate(() => window.remoteControl.snapshot())).value;
  check(state.devices.length === 1 && state.devices[0].peerId === '321654987', 'repeated save clicks produce exactly one normalized device');
  check(state.session.phase === 'idle' && !(await page.locator('#consent').isChecked()), 'finishing the wizard neither launches nor pre-approves a session');
  await page.getByRole('button', { name: '移除 我的 Mac · 向导测试', exact: true }).click();
  await page.waitForFunction(() => document.getElementById('device-count').textContent === '0');
  for (let step = 0; step < 4; step++) {
    await page.locator('#start-guide').click();
    for (let i = 0; i < step; i++) {
      if (i === 2) { await page.locator('#wizard-name').fill('取消测试'); await page.locator('#wizard-id').fill('123123123'); }
      await page.locator('#wizard-next').click();
    }
    await page.locator('#wizard-cancel').click();
    check((await page.evaluate(() => window.remoteControl.snapshot())).value.devices.length === 0, 'cancel at wizard step ' + (step + 1) + ' leaves no device behind');
  }
};
