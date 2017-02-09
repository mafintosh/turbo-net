process.title = 'turbo-echo-server'

var turbonet = require('../')
var pretty = require('prettier-bytes')
var speed = require('speedometer')()

var server = turbonet.createServer(function (socket) {
  console.log('socket.remoteAddress', socket.remoteAddress())

  socket.on('pipe-buffer-increase', function (size) {
    console.log('new buffer size:', size)
  })

  socket.pipe(socket, function (err) {
    socket.destroy()
    console.log('end of pipe:', err)
  })

  socket.on('data', function (data) {
    speed(data.length)
  })
})

server.listen(20000, function () {
  console.log('listening on', server.address())
})

setInterval(function () {
  console.log('echo throughput:', pretty(speed()) + '/s')
}, 1000)
