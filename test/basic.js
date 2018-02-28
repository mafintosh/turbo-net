const tape = require('tape')
const turbo = require('../')

tape('basic', function (t) {
  const opts = {allowHalfOpen: true}

  const server = turbo.createServer(opts, onsocket)

  server.listen(0, function () {
    const socket = turbo.connect(server.address().port, opts)
    const chunks = []

    socket.read(Buffer.alloc(3), function onread (err, buf, n) {
      t.error(err, 'no error')
      chunks.push(buf.slice(0, n))
      if (n) return socket.read(Buffer.alloc(3), onread)
      socket.close()
      server.close()
      t.same(Buffer.concat(chunks), Buffer.from('abc'))
      t.end()
    })

    socket.write(Buffer.from('a'))
    socket.write(Buffer.from('b'))
    socket.write(Buffer.from('c'))
    socket.end()
  })

  function onsocket (socket) {
    socket.read(Buffer.alloc(3), function onread (err, buf, read) {
      if (!read) return socket.end()
      t.error(err, 'no error')
      socket.write(buf, read, function () {
        socket.read(buf, onread)
      })
    })
  }
})
