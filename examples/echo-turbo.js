const turbo = require('../')
const speedometer = require('speedometer')

const server = turbo.createServer()

server.on('connection', function (c) {
  pipe(c, c)
})

server.listen(8080, function () {
  const c = turbo.connect(8080, 'localhost')
  const buffer = Buffer.alloc(1024 * 1024)
  const speed = speedometer()

  c.read(buffer, function onread (err, buf, len) {
    if (err) throw err
    speed(len)
    c.read(buffer, onread)
  })

  for (var i = 0; i < 8; i++) {
    c.write(Buffer.alloc(1024 * 1024), 1024 * 1024, onwrite)
  }

  setInterval(() => console.log(speed()), 1000)

  function onwrite (err, buf) {
    if (err) throw err
    c.write(buf, buf.length, onwrite)
  }
})

function pipe (a, b) {
  let bufferSize = 16 * 1024
  let max = 8 // up to 8mb
  let full = 4

  for (var i = 0; i < 4; i++) {
    a.read(Buffer.allocUnsafe(bufferSize), onread)
  }

  function onread (_, buf, n) {
    if (!n) return

    if (n === buf.length) {
      if (!--full && max) {
        full = 4
        bufferSize *= 2
        max--
      }
    } else {
      full = 4
    }

    b.write(buf, n, onwrite)
  }

  function onwrite (err, buf, n) {
    if (err) return
    if (buf.length < bufferSize) buf = Buffer.allocUnsafe(bufferSize)
    a.read(buf, onread)
  }
}
