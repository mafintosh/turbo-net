const tape = require('tape')
const turbo = require('../')
const os = require('os')
const semver = require('semver')

tape('listen', function (t) {
  const server = turbo.createServer()

  server.listen(function () {
    const addr = server.address()
    t.ok(typeof addr.port === 'number')
    t.same(addr.family, 'IPv4')
    t.same(addr.address, '0.0.0.0')
    server.close(function () {
      server.listen(addr.port, function () {
        t.same(server.address(), addr)
        server.close()
        t.end()
      })
    })
  })
})

tape('listen stringed port', function (t) {
  const server = turbo.createServer()

  server.listen(function () {
    const addr = server.address()
    server.close(function () {
      server.listen('' + addr.port, function () {
        t.same(server.address(), addr)
        server.close()
        t.end()
      })
    })
  })
})

tape('address no listen', function (t) {
  const server = turbo.createServer()

  try {
    server.address()
  } catch (err) {
    t.pass('should error')
    t.end()
  }
})

tape('listen on used port', function (t) {
  const server = turbo.createServer({
    reusePort: false
  })

  server.listen(function () {
    const another = turbo.createServer({
      reusePort: false
    })

    another.on('error', function (err) {
      server.close()
      t.ok(err, 'had error')
      t.end()
    })

    another.listen(server.address().port)
  })
})

tape(`listen on used port (SO_REUSEPORT) (${os.platform()}:${os.release()})`, function (t) {
  if (os.platform() === 'linux' && !semver.satisfies(semver.coerce(os.release()), '>=3.9')) {
    t.pass('SO_REUSEPORT only supported on kernel 3.9+')
    t.end()
    return
  }

  const server = turbo.createServer()

  server.listen(function () {
    const another = turbo.createServer()

    another.listen(server.address().port, function () {
      server.close()
      another.close()
      t.pass('should not error')
      t.end()
    })
  })
})
