# turbo-net

Experimental TCP library for Node.js.
Focuses on performance and low memory footprint by allowing easy reuse of buffers while still being easy to use.

## CURRENT STATUS: WIP AND UNSTABLE

## Usage

``` js
var net = require('turbo-net')

var server = net.createServer(function (socket) {
  var buffer = new Buffer(65536) // allocate a buffer

  socket.read(buffer, function onread (err, data) {
    if (err) return console.log('error!', err)
    if (!data) return socket.end()
    console.log('data:', data.toString())
    socket.read(buffer, onread)
  })
})

server.listen(10000, function () {
  console.log('Server is listening on port 10000')
})
```

Try running the example echo server in `./example/server.js`
and the client `./example/client.js`.

The client will write data as fast as possible to the server and print out the throughput
Both the client and server are reusing buffers, keeping the memory usage flat.

On my laptop this gets `~1.4GB/s` with 16MB ram used.

## API

#### `var server = net.createServer([onconnection])`

Create a new server. Optionally you can pass a onconnection handler.

#### `server.listen([port], [onlistening])`

Listen on a port. If port is omitted or is 0 a random port will be chosen.

#### `server.on('connection', socket)`

Emitted when a new socket connects

#### `server.on('listening')`

Emitted when the server is listening

#### `server.connections`

An array of all connections the server currently has (order not guaranteed).

#### `var socket = net.connect(port, [host])`

Connect to a server.

#### `socket.read([buffer], callback)`

Read data. Optionally you can pass in a buffer to read the data into.
The callback is always called and will contain an error if the read failed.
If no more data is available the callback will be called with `(null, null)`
otherwise `(null, data)` where data is the data read.

#### `socket.write(buffer, [callback])`

Write data. Callback is called when the write completed and is guaranteed to be
called.

#### `socket.end([callback])`

End the socket. Waits for all pending data to be flushed and then destroys it and calls the
callback.

#### `socket.destroy([err], [callback])

Destroy the socket. Optionally pass an error that is passed to pending callbacks.
If a read/write error occurs the socket is automatically destroyed but this method is
safe to call more than once.

#### `socket.on('close')`

Emitted when a destroy has been completed and the socket is fully closed

#### `socket.on('end')`

Emitted when a read returned `null` signalling no more data.

#### `socket.on('finish')`

Emitted when end has been called and all pending writes flushed.
