const tape = require('tape')
const proc = require('child_process')
const turbo = require('../')

tape('uncaughts are not swallowed', function (t) {
  const server = turbo.createServer(socket => socket.end())

  server.listen(function () {
    const client = turbo.connect(server.address().port)

    process.on('uncaughtException', function (err) {
      client.close()
      server.close()
      t.same(err.message, 'stop')
      t.end()
    })

    client.on('connect', function () {
      throw new Error('stop')
    })
  })
})

tape('uncaughts are not swallowed (child process)', function (t) {
  const child = proc.spawn(process.execPath, ['-e', `
    const turbo = require('../')
    const server = turbo.createServer(socket => socket.end())

    server.listen(function () {
      const client = turbo.connect(server.address().port)
      client.on('connect', function () {
        throw new Error('stop')
      })
    })
  `], {
    cwd: __dirname
  })

  const buf = []
  child.stderr.on('data', data => buf.push(data))
  child.stderr.on('end', function () {
    t.ok(buf.join('').indexOf('Error: stop') > -1)
    t.end()
  })
})
