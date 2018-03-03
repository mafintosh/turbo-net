const tape = require('tape')
const turbo = require('../')

tape('connect with hostname', function (t) {
  const server = turbo.createServer(socket => socket.end())

  server.listen(function () {
    const client = turbo.connect(server.address().port, 'localhost')

    client.on('connect', function () {
      server.close()
      t.pass('connected')
      t.end()
    })
  })
})

tape('connect with error', function (t) {
  const server = turbo.createServer(socket => socket.end())

  server.listen(function () {
    const port = server.address().port

    server.close(function () {
      const client = turbo.connect(port, 'localhost')

      client.on('error', function (err) {
        t.ok(err, 'should error')
        t.end()
      })
    })
  })
})

tape('connect with and read/write and error', function (t) {
  t.plan(3)

  const server = turbo.createServer(socket => socket.end())

  server.listen(function () {
    const port = server.address().port

    server.close(function () {
      const client = turbo.connect(port, 'localhost')

      client.on('error', err => t.ok(err, 'should error'))
      client.read(Buffer.alloc(1024), err => t.ok(err, 'should error'))
      client.write(Buffer.alloc(1024), err => t.ok(err, 'should error'))
    })
  })
})

tape('close before connect', function (t) {
  const server = turbo.createServer(socket => socket.end())

  server.listen(function () {
    const port = server.address().port
    const client = turbo.connect(port)

    client.close(function () {
      console.log('callback')
      server.close()
      t.end()
    })
  })
})
