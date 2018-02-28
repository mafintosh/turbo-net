const dns = require('dns')
const IPv4 = /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/

module.exports = lookup

function lookup (addr, cb) {
  if (IPv4.test(addr)) return process.nextTick(cb, null, addr)
  dns.lookup(addr, cb)
}
