'use strict';
const { isIP } = require('node:net');

// Local adapter facts only. Neither an address nor an adapter name proves that
// a VPN is authenticated, a peer is reachable, or a session is encrypted.
function describeInterfaces(interfaces) {
  const entries = [];
  for (const [name, addresses] of Object.entries(interfaces || {})) {
    for (const entry of addresses || []) {
      const family = isIP(entry.address || '');
      if (entry.internal || !family) continue;
      const bytes = entry.address.split('.').map(Number);
      const privateV4 = family === 4 && (bytes[0] === 10 || (bytes[0] === 172 && bytes[1] >= 16 && bytes[1] <= 31) || (bytes[0] === 192 && bytes[1] === 168));
      const kind = /tailscale/i.test(name) ? 'tailscale-adapter' : privateV4 ? 'private-address' : 'other-address';
      entries.push({ name, address: entry.address, family: 'IPv' + family, kind });
    }
  }
  return entries;
}
module.exports = { describeInterfaces };
