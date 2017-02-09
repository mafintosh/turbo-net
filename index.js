var binding = require('node-gyp-build')(__dirname)
var set = require('unordered-set')
var util = require('util')
var events = require('events')

exports.createServer = function (onconnection) {
  var s = new Server()
  if (onconnection) s.on('connection', onconnection)
  return s
}

exports.connect = function (port, host) {
  if (typeof port === 'string') port = parseInt(port, 10)
  if (typeof port !== 'number' || !port) throw new Error('port is required')
  if (!host) host = '127.0.0.1'

  var c = new Connection()
  c._connect(port, host)
  return c
}

function Server () {
  events.EventEmitter.call(this)

  this.connections = []

  this.handle = binding.create()
  this.handle.context(this)
  this.handle.onconnection(this._onconnection)
}

util.inherits(Server, events.EventEmitter)

Server.prototype.address = function () {
  return this.handle.address()
}

Server.prototype.listen = function (port, onlistening) {
  if (typeof port === 'string') port = parseInt(port, 10)
  if (typeof port === 'function') return this.listen(0, port)
  if (typeof port !== 'number' || isNaN(port)) throw new Error('port is required')

  if (onlistening) this.once('listening', onlistening)

  var self = this

  process.nextTick(function () {
    self.handle.listen(port, '127.0.0.1')
    self.emit('listening')
  })
}

Server.prototype._onconnection = function (handle) {
  var c = new Connection()
  c._server = this
  c._onhandle(handle)
  set.add(this.connections, c)
  this.emit('connection', c)
}

function Connection () {
  events.EventEmitter.call(this)

  this._socketHandle = null
  this._handle = null
  this._index = -1
  this._server = null

  this._maxBuffer = 1024 * 1024
  this._reads = []
  this._writes = []
  this._pendingWrites = []
  this._pendingReads = []
  this._pendingEnds = []
  this._pendingCloses = []
  this._queued = []
  this._emitData = false
  this._emitError = false

  this._error = null
  this._finished = false
  this._closed = false
  this._ended = false
  this._connected = false
  this._error = null

  this.destroyed = false
  this.writable = true
  this.readable = true

  this.on('newListener', this._onlistener)
}

util.inherits(Connection, events.EventEmitter)

Connection.prototype.address = function () {
  if (!this._handle) throw new Error('Cannot get address until socket is connected')
  return this._handle.address()
}

Connection.prototype.remoteAddress = function () {
  if (!this._handle) throw new Error('Cannot get remote address until socket is connected')
  return this._handle.remoteAddress()
}

Connection.prototype._onlistener = function (name) {
  if (name === 'data') this._emitData = true
  if (name === 'error') this._emitError = true
}

Connection.prototype._onhandle = function (handle) {
  this._handle = handle
  this._handle.context(this)
  this._handle.onread(this._onread)
  this._handle.onend(this._onend)
  this._handle.onwrite(this._onwrite)
  this._handle.onfinish(this._onfinish)
  this._handle.onerror(this._onerror)
  this._handle.onclose(this._onclose)
}

Connection.prototype._connect = function (port, host) {
  this._socketHandle = binding.create()
  this._socketHandle.context(this)
  this._socketHandle.onconnect(this._onconnect)
  this._socketHandle.connect(port, host)
}

Connection.prototype.read = function (buf, cb) {
  if (typeof buf === 'function') return this.read(new Buffer(16 * 1024), buf)
  if (!Buffer.isBuffer(buf) || buf.length === 0) throw new Error('Can only read into a non-empty buffer')
  if (!this.readable) return process.nextTick(cb, new Error('No longer readable'))

  this._queueRead(new IORequest(buf, null, cb || noop))
}

Connection.prototype.write = function (buf, cb) {
  if (!cb) cb = noop
  if (!this.writable || this._ended) return process.nextTick(cb, new Error('No longer writable'))

  if (Buffer.isBuffer(buf)) return this._write(buf, cb)
  if (typeof buf === 'string') return this._write(new Buffer(buf), cb)
  if (Array.isArray(buf)) return this._writeAll(buf, cb)

  throw new Error('Can only write a string, buffer, or array of buffers')
}

Connection.prototype.end = function (cb) {
  if (!cb) cb = noop
  if (!this.destroyed) return process.nextTick(cb, new Error('No longer writable'))

  if (this._finished) return process.nextTick(cb)
  this._pendingEnds.push(cb)

  if (this._ended) return
  this._ended = true
  if (!this._writes.length) this._handle.end()
}

Connection.prototype._write = function (buf, cb) {
  this._queueWrite(new IORequest(buf, null, cb))
}

Connection.prototype._writeAll = function (bufs, cb) {
  this._queueWrite(new IORequest(null, bufs, cb))
}

Connection.prototype.pipe = function (dest, opts, cb) {
  if (typeof opts === 'function') return this.pipe(dest, null, opts)
  if (!opts) opts = {}
  return this._pipe(dest, opts.bufferSize || 4096, cb || noop)
}

Connection.prototype.destroy = function (err, cb) {
  if (typeof err === 'function') return this.destroy(null, err)
  if (!cb) cb = noop

  if (this._closed) return process.nextTick(cb)
  this._pendingCloses.push(cb)

  if (this.destroyed) return
  this._error = err
  this.destroyed = true
  this.readable = false
  this.writable = false
  this._handle.close()
}

Connection.prototype._pipe = function (dest, size, cb) {
  var src = this
  var writes = []
  var buffers = []
  var flushing = 0
  var called = false

  for (var i = 0; i < 4; i++) {
    src.read(new Buffer(size), onread)
  }

  return dest

  function done (err) {
    if (called) return
    called = true

    if (!err) return dest.end(cb)
    src.destroy(err, function () {
      dest.destroy(err, cb)
    })
  }

  function onread (err, data, buffer) {
    if (err) return done(err)
    if (!data) return done(null)

    if (buffer.length === data.length && size < src._maxBuffer) {
      src.emit('pipe-buffer-increase', size *= 2)
      for (var i = 0; i < 4; i++) {
        src.read(new Buffer(size), onread)
      }
    }

    buffers.push(buffer)
    if (writes.push(data) === 1) write()
  }

  function write (err) {
    if (err) return done(err)

    flushing = writes.length
    dest.write(writes, onwrite)
  }

  function onwrite (err) {
    if (err) return done(err)

    for (var i = 0; i < flushing; i++) {
      var buf = buffers[i]
      if (buf.length === size) continue
      src.read(buf, onread)
    }

    while (flushing--) {
      writes.shift()
      buffers.shift()
    }

    if (writes.length) write()
  }
}

Connection.prototype._queueRead = function (req) {
  if (this._reads.length === 64) {
    this._pendingReads.push(req)
    return
  }

  this._reads.push(req)
  if (req.buffer) this._handle.read(req.buffer)
  else this._handle.readAll(req.bufferList)
}

Connection.prototype._queueWrite = function (req) {
  if (this._writes.length === 64) {
    this._pendingWrites.push(req)
    return
  }

  this._writes.push(req)
  if (req.buffer) this._handle.write(req.buffer)
  else this._handle.writeAll(req.bufferList)
}

Connection.prototype._onclose = function () {
  if (this._server) set.remove(this._server.connections, this)
  this._handle = null
  this._socketHandle = null

  var err = this._error
  this._error = null
  if (this._emitError) this.emit('error', err)
  this.emit('close')

  drain(this._reads, err)
  drain(this._pendingReads, err)
  drain(this._writes, err)
  drain(this._pendingWrites, err)
  while (this._pendingEnds.length) this._pendingEnds.shift()(err)
  while (this._pendingCloses.length) this._pendingCloses.shift()(err)
}

Connection.prototype._onread = function (len) {
  var req = this._reads.shift()
  if (this._pendingReads.length && this.readable) this._queueReads(this._pendingReads.shift())
  var data = req.buffer.slice(0, len)
  if (this._emitData) this.emit('data', data)
  req.callback(null, data, req.buffer)
}

Connection.prototype._onwrite = function () {
  var req = this._writes.shift()
  if (this._pendingWrites.length && this.writable) this._queueWrite(this._pendingWrites.shift())
  if (!this._writes.length && this.writable && this._ended) this.handle.end()
  req.callback(null)
}

Connection.prototype._onerror = function (code) {
  this.destroy(error(code))
}

Connection.prototype._onend = function () {
  this.readable = false
  drain(this._reads, null)
  drain(this._pendingReads, null)
}

Connection.prototype._onfinish = function () {
  this._finished = true
  this.writable = false
  this.emit('finish')
  while (this._pendingEnds.length) this._pendingEnds.shift()(null)
  this.destroy()
}

Connection.prototype._onconnect = function (handle) {
  this._onhandle(handle)
  this._connected = true
  this.emit('connect')
}

function IORequest (buf, list, cb) {
  this.buffer = buf
  this.bufferList = list
  this.callback = cb
}

function noop () {}

function error (code) {
  var err = new Error(code)
  err.code = code
  return err
}

function drain (list, err) {
  while (list.length) {
    var req = list.shift()
    req.callback(err, null, req.buffer)
  }
}
