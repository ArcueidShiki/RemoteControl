'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { describeInterfaces } = require('../src/network.cjs');
test('local adapter labels do not infer VPN trust, peer connectivity or transport from addresses', () => {
  const result = describeInterfaces({
    Loopback: [{ address: '127.0.0.1', internal: true }],
    Ethernet: [{ address: '192.168.1.2' }, { address: '100.64.1.2' }, { address: '203.0.113.7' }],
    Tailscale: [{ address: '100.64.1.3' }, { address: 'fd7a:115c:a1e0::1' }],
    Disabled: null, Malformed: [{ address: 'not an address' }]
  });
  assert.equal(result.length, 5);
  assert.equal(result[0].kind, 'private-address');
  assert.equal(result[1].kind, 'other-address', 'CGNAT range alone is not Tailscale evidence');
  assert.equal(result[3].kind, 'tailscale-adapter');
  assert.equal(result[4].family, 'IPv6');
  for (const item of result) assert.deepEqual(Object.keys(item), ['name', 'address', 'family', 'kind']);
});
