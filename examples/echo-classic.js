const net = require('net')
const speedometer = require('speedometer')

const server = net.createServer(function (socket) {
  socket.pipe(socket)
})

server.listen(8080, function () {
  const speed = speedometer()
  const socket = net.connect(8080)
  const range = Array(8).join(',').split(',')

  socket.on('data', function (data) {
    speed(data.length)
  })

  range.forEach(function () {
    const buf = Buffer.alloc(1024 * 1024)
    socket.write(buf, function onwrite () {
      socket.write(buf, onwrite)
    })
  })

  setInterval(() => console.log(speed()), 1000)
})
