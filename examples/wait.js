const turbo = require('../')

const server = turbo.createServer(function (socket) {
  console.log(socket.connection.remoteAddress, socket.connection.remotePort)

  socket.close()
})

server.listen(8080)
