const tape = require('tape')
const turbo = require('../')

tape('writev', function (t) {
  const server = turbo.createServer(echo)

  server.listen(function () {
    const client = turbo.connect(server.address().port)

    read(client, 11, function (err, buf) {
      t.error(err, 'no error')
      t.same(buf, Buffer.from('hello world'))
      server.close()
      client.close(() => t.end())
    })

    client.writev([
      Buffer.from('hello '),
      Buffer.from('world')
    ])
  })
})

tape('writev after connect', function (t) {
  const server = turbo.createServer(echo)

  server.listen(function () {
    const client = turbo.connect(server.address().port)

    read(client, 11, function (err, buf) {
      t.error(err, 'no error')
      t.same(buf, Buffer.from('hello world'))
      server.close()
      client.close(() => t.end())
    })

    client.on('connect', function () {
      client.writev([
        Buffer.from('hello '),
        Buffer.from('world')
      ])
    })
  })
})

tape('writev before and after connect', function (t) {
  const server = turbo.createServer(echo)

  server.listen(function () {
    const client = turbo.connect(server.address().port)

    read(client, 14 + 11, function (err, buf) {
      t.error(err, 'no error')
      console.log(buf.toString())
      t.same(buf, Buffer.from('hej verden og hello world'))
      server.close()
      client.close(() => t.end())
    })

    client.writev([
      Buffer.from('hej '),
      Buffer.from('verden '),
      Buffer.from('og ')
    ])

    client.on('connect', function () {
      client.writev([
        Buffer.from('hello '),
        Buffer.from('world')
      ])
    })
  })
})

tape('writev twice', function (t) {
  const server = turbo.createServer(echo)

  server.listen(function () {
    const client = turbo.connect(server.address().port)

    read(client, 14 + 11, function (err, buf) {
      t.error(err, 'no error')
      t.same(buf, Buffer.from('hej verden og hello world'))
      server.close()
      client.close(() => t.end())
    })

    client.writev([
      Buffer.from('hej '),
      Buffer.from('verden '),
      Buffer.from('og ')
    ])

    client.writev([
      Buffer.from('hello '),
      Buffer.from('world')
    ])
  })
})

tape('write 256 buffers', function (t) {
  const server = turbo.createServer(echo)

  server.listen(function () {
    const client = turbo.connect(server.address().port)
    const expected = Buffer.alloc(256)

    read(client, 256, function (err, buf) {
      t.error(err, 'no error')
      t.same(buf, expected)
      server.close()
      client.close(() => t.end())
    })

    for (var i = 0; i < 256; i++) {
      expected[i] = i
      client.write(Buffer.from([i]))
    }
  })
})

function read (socket, read, cb) {
  const buf = Buffer.alloc(read)
  socket.read(buf, function (err, next, n) {
    if (err) return cb(err)
    read -= n
    if (!read) return cb(null, buf)
    socket.read(next.slice(n), cb)
  })
}

function echo (socket) {
  socket.read(Buffer.alloc(65536), function onread (err, buf, n) {
    if (err) return
    socket.write(buf, n, function (err) {
      if (err) return
      socket.read(buf, onread)
    })
  })
}
