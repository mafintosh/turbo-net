const binding = require('./binding')
const Connection = require('./connection')
const lookup = require('./lookup')
const events = require('events')

class Server extends events.EventEmitter {
  constructor (opts) {
    if (!opts) opts = {}
    super()

    this.connections = []
    this.allowHalfOpen = !!opts.allowHalfOpen

    this._closed = false
    this._address = null
    this._handle = null
  }

  address () {
    if (!this._address) throw new Error('Not bound')
    return {
      address: this._address,
      family: 'IPv4',
      port: binding.turbo_net_tcp_port(this._handle)
    }
  }

  close (onclose) {
    if (!this._address) return
    if (onclose) this.once('close', onclose)
    if (this._closed) return
    this._closed = true
    binding.turbo_net_tcp_close(this._handle)
  }

  listen (port, address, backlog, onlistening) {
    if (typeof port === 'function') return this.listen(0, null, port)
    if (typeof address === 'function') return this.listen(port, null, address)
    if (typeof backlog === 'function') return this.listen(port, address, 511, backlog)
    if (!port) port = 0
    if (typeof port !== 'number') port = Number(port)

    if (onlistening) this.once('listening', onlistening)

    const self = this

    lookup(address || '0.0.0.0', function (err, ip) {
      if (err) return self.emit('error', err)
      if (self._address) self.emit('error', new Error('Already bound'))

      self._init()

      try {
        binding.turbo_net_tcp_listen(self._handle, port, ip, backlog)
      } catch (err) {
        self.emit('error', err)
      }

      self._address = ip
      self.emit('listening')
    })
  }

  _init () {
    if (this._handle) return

    this._handle = Buffer.alloc(binding.sizeof_turbo_net_tcp_t)

    binding.turbo_net_tcp_init(this._handle, this,
      this._onallocconnection,
      null,
      null,
      null,
      null,
      this._onclose
    )
  }

  _onclose () {
    this._closed = false
    this._address = null
    binding.turbo_net_tcp_destroy(this._handle)
    this._handle = null
    this.emit('close')
  }

  _onallocconnection () {
    var c = new Connection(this)
    return c._handle
  }
}

module.exports = Server
