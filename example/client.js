process.title = 'turbo-echo-client'

var turbonet = require('../')
var pretty = require('prettier-bytes')
var speed = require('speedometer')()

var socket = turbonet.connect(20000)

socket.on('data', function (data) {
  speed(data.length)
})

socket.on('connect', function () {
  console.log('socket.address', socket.address())
  console.log('socket.remoteAddress', socket.remoteAddress())

  socket.on('pipe-buffer-increase', function (size) {
    console.log('new buffer size:', size)
  })

  socket.pipe(socket, function (err) {
    console.log('end of pipe', err)
  })

  socket.write(new Buffer(8 * 65536))
  socket.write(new Buffer(8 * 65536))
  socket.write(new Buffer(8 * 65536))
  socket.write(new Buffer(8 * 65536))
})

setInterval(function () {
  console.log('echo throughput:', pretty(speed()) + '/s')
}, 1000)
