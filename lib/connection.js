const binding = require('./binding')
const events = require('events')
const unordered = require('unordered-set')
const RequestQueue = require('./queue')
const lookup = require('./lookup')

const EMPTY = Buffer.alloc(0)

class Connection extends events.EventEmitter {
  constructor (server) {
    super()

    this.closed = false
    this.finished = false
    this.ended = false
    this.allowHalfOpen = false
    this.writable = false
    this.readable = false

    this._index = 0 // used be unordered set
    this._server = null
    this._handle = Buffer.alloc(binding.sizeof_turbo_net_tcp_t)
    this._reads = new RequestQueue(8, 0)
    this._writes = new RequestQueue(16, binding.sizeof_uv_write_t)

    this._finishing = []
    this._closing = []
    this._paused = true
    this._queued = server ? null : []

    binding.turbo_net_tcp_init(this._handle, this,
      null,
      this._onconnect,
      this._onwrite,
      this._onread,
      this._onfinish,
      this._onclose
    )

    if (server) {
      this._server = server
      this._index = server.connections.push(this) - 1
      this.allowHalfOpen = server.allowHalfOpen
    }
  }

  _connect (port, host) {
    const self = this

    lookup(host, function (err, ip) {
      if (err) return self.emit('error', err)
      binding.turbo_net_tcp_connect(self._handle, port, ip)
    })
  }

  _onclose () {
    if (this._server) unordered.remove(this._server.connections, this)
    this.closed = true

    this._closing = callAll(this._closing, null)
    if (this._reads.top !== this._reads.btm) this._onend(new Error('Closed'))

    binding.turbo_net_tcp_destroy(this._handle)
    this._handle = this._server = null

    this.emit('close')
  }

  _onfinish (status) {
    this.finished = true
    if (this.ended || !this.allowHalfOpen) this.close()
    this.emit('finish')

    const err = status < 0 ? new Error('End failed') : null
    if (err) this.close()
    this._finishing = callAll(this._finishing, err)
  }

  _onend (err) {
    while (this._reads.top !== this._reads.btm) this._reads.shift().done(err, 0)
    if (err) return
    if (this.finished || !this.allowHalfOpen) this.close()
    this.emit('end')
  }

  _onconnect (status) {
    if (status < 0) {
      this.emit('error', new Error('Connect failed'))
    } else {
      this.readable = true
      this.writable = true
      if (this._server) this._server.emit('connection', this)
      else this.emit('connect')
    }
    if (this._queued) this._unqueue()
  }

  _unqueue () {
    const queued = this._queued
    this._queued = null
    while (queued.length) {
      const [cmd, data, len, cb] = queued.shift()
      this._call(cmd, data, len, cb)
    }
  }

  _call (cmd, data, len, cb) {
    switch (cmd) {
      case 0: return this.write(data, len, cb)
      case 1: return this.writev(data, len, cb)
      case 2: return this.end(cb)
      case 3: return this.read(data, cb)
      case 4: return this.close(cb)
    }
  }

  _onread (read) {
    if (!read) {
      this.readable = false
      this.ended = true
      this._onend(null)
      return EMPTY
    }

    const reading = this._reads.shift()
    const err = read < 0 ? new Error('Read failed') : null

    if (err) {
      this.close()
      reading.done(err, 0)
      return EMPTY
    }

    reading.done(err, read)

    if (this._reads.top === this._reads.btm) {
      this._paused = true
      return EMPTY
    }

    return this._reads.peek().buffer
  }

  _onwrite (status) {
    const writing = this._writes.shift()
    const err = status < 0 ? new Error('Write failed') : null

    if (err) {
      this.close()
      writeDone(writing, err)
      return
    }

    writeDone(writing, null)
  }

  write (data, len, cb) {
    if (typeof len === 'function') this._write(data, data.length, len)
    else if (!len) this._write(data, data.length, cb || noop)
    else this._write(data, len, cb || noop)
  }

  writev (datas, lens, cb) {
    if (typeof lens === 'function') this._writev(datas, getLengths(datas), lens)
    else if (!lens) this._writev(datas, getLengths(datas), cb || noop)
    else this._writev(datas, lens, cb || noop)
  }

  _writev (datas, lens, cb) {
    if (!this.writable) return this._notWritable(cb, datas, lens)
    const writing = this._writes.push()

    writing.buffers = datas
    writing.lengths = lens
    writing.callback = cb

    if (datas.length === 2) { // faster c case for just two buffers which is common
      binding.turbo_net_tcp_write_two(this._handle, writing.handle, datas[0], lens[0], datas[1], lens[1])
    } else {
      binding.turbo_net_tcp_writev(this._handle, writing.handle, datas, lens)
    }
  }

  _write (data, len, cb) {
    if (!this.writable) return this._notWritable(cb, data, len)
    const writing = this._writes.push()

    writing.buffer = data
    writing.length = len
    writing.callback = cb

    binding.turbo_net_tcp_write(this._handle, writing.handle, writing.buffer, len)
  }

  close (cb) {
    if (!cb) cb = noop
    if (this.closed) return process.nextTick(cb)

    if (this._queued) {
      this._queued.push([4, null, cb])
      return
    }

    this._closing.push(cb)
    if (this._closing.length > 1) return

    this.readable = this.writable = false
    binding.turbo_net_tcp_close(this._handle)
  }

  end (cb) {
    if (!cb) cb = noop
    if (!this.writable) return this._notWritable(cb)

    this._finishing.push(cb)
    if (this._finishing.length > 1) return

    this.writable = false
    binding.turbo_net_tcp_shutdown(this._handle)
  }

  read (data, cb) {
    if (!this.readable) return this._notReadable(cb, data)

    const reading = this._reads.push()

    reading.buffer = data
    reading.callback = cb

    if (this._paused) {
      this._paused = false
      binding.turbo_net_tcp_read(this._handle, data)
    }
  }

  _notWritable (cb, data, len) {
    if (this._queued) {
      const type = data ? (Array.isArray(data) ? 1 : 0) : 2
      this._queued.push([type, data, len || 0, cb])
      return
    }
    process.nextTick(cb, this.finished ? null : new Error('Not writable'), data)
  }

  _notReadable (cb, data) {
    if (this._queued) {
      this._queued.push([3, data, 0, cb])
      return
    }
    process.nextTick(cb, this.ended ? null : new Error('Not readable'), data, 0)
  }
}

module.exports = Connection

function noop () {}

function callAll (list, err) {
  for (var i = 0; i < list.length; i++) list[i](err)
  return null
}

function getLengths (datas) {
  var lens = new Array(datas.length)
  for (var i = 0; i < datas.length; i++) lens[i] = datas[i].length
  return lens
}

function writeDone (req, err) {
  if (req.buffers) req.donev(err)
  else req.done(err, req.length)
}
