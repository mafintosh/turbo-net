const tape = require('tape')
const turbo = require('../')

tape('read', function (t) {
  const server = turbo.createServer(function (socket) {
    socket.write(Buffer.from('hello world'))
    socket.end()
  })

  server.listen(function () {
    const socket = turbo.connect(server.address().port)

    socket.on('connect', function () {
      socket.read(Buffer.alloc(1024), function (err, buf, n) {
        t.error(err, 'no error')
        t.ok(n > 0)
        t.same(buf.slice(0, n), Buffer.from('hello world').slice(0, n))
        socket.close()
        server.close()
        t.end()
      })
    })
  })
})

tape('many reads', function (t) {
  const expected = Buffer.from('hello world hello world hello world')
  const server = turbo.createServer(function (socket) {
    socket.write(expected)
    socket.end()
  })

  t.plan(2 * expected.length + 2)

  server.listen(function () {
    const socket = turbo.connect(server.address().port)
    for (var i = 0; i < expected.length; i++) {
      const next = expected[i]
      socket.read(Buffer.alloc(1), function (err, buf) {
        t.error(err)
        t.same(buf, Buffer.from([next]))
      })
    }
    socket.read(Buffer.alloc(1024), function (err, buf, n) {
      server.close()
      t.error(err)
      t.same(n, 0)
    })
  })
})
