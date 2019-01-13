# turbo-net

Low level TCP library for Node.js

```
npm install turbo-net
```

[![build status](https://travis-ci.org/mafintosh/turbo-net.svg?branch=master)](https://travis-ci.org/mafintosh/turbo-net)
[![Build status](https://ci.appveyor.com/api/projects/status/1rbh090naan36163/branch/master?svg=true)](https://ci.appveyor.com/project/mafintosh/turbo-net/branch/master)

## Usage

``` js
const turbo = require('turbo-net')

// Echo server that allocates constant memory

const server = turbo.createServer(function (socket) {
  socket.read(Buffer.alloc(32 * 1024), function onread (err, buf, read) {
    if (err) throw err
    socket.write(buf, read, function (err) {
      if (err) throw err
      socket.read(buf, onread)
    })
  })
})

server.listen(8080, function () {
  const socket = turbo.connect(8080)

  socket.read(Buffer.alloc(32), function (err, buf, read) {
    if (err) throw err
    console.log(buf.toString('utf-8', 0, read))
  })
  socket.write(Buffer.from('hello world\n'))
})
```

## Performance

Running the echo server examples in `./examples` I get the following throughput on my laptop

* echo-classic: 1.3GB/s at ~100MB of ram
* echo-turbo: 3.4GB/s at ~35MB of ram

## API

#### `server = turbo.createServer([options], [onsocket])`

Create a new TCP server. Options include:

``` js
{
  allowHalfOpen: false // set to true to allow half open TCP connections
}
```

#### `server.on('connection', connection)`

Emitted when a new connection is established.

#### `server.on('listening')`

Emitted when the server is listening.

#### `server.connections`

Unordered array containing the current active connections

#### `server.listen(port, [address], [onlistening])`

Listen on a port.

#### `server.address()`

Similar to net.Server.address. Useful if you are listening on port 0,
to find out which port was picked.

#### `server.close([onclose])`

Close the server.

#### `connection = turbo.connect(port, host, [options])`

Connect to a TCP server. Options include:

``` js
{
  allowHalfOpen: false // set to true to allow half open TCP connections
}
```

#### `connection.on('connect')`

Emitted when a client connection is fully connected.

#### `connection.on('error', err)`

Emitted when a client fails to connect.

#### `connection.on('close')`

Emitted a connection is fully closed. No other events will be emitted after.

#### `connection.on('finish')`

Emitted when the writable side is fully closed.

#### `connection.on('end')`

Emitted when the readable side is fully closed.

#### `connection.on('timeout')`

Emitted when the timeout it's reached. Is only a notification, you should end the connection.

#### `connection.close([callback])`

Closes the connection.

#### `connection.read(buffer, callback)`

Read data from the connection. Data will be read into the buffer you pass.

The callback is called with `callback(err, buffer, bytesRead)`.

If `bytesRead` is `0`, then the readable side of the connection has ended.

#### `connection.write(buffer, [length], [callback])`

Write data to the connection. Optionally you specify how many bytes in the
buffer you want to write.

The callback is called with `callback(err, buffer, length)`.

#### `connection.writev(buffers, [lengths], [callback])`

Write more than one buffer at once. Optionally you can specify how many bytes in
each buffer you want to write.

The callback is called with `callback(err, buffers, lengths)`.

#### `connection.end([callback])`

End the writable side of the connection.

#### `connection.setTimeout(millis, [callback])`

Set timeout on the connection. Optionally you can provide an callback. If millis is setted on 0, then it disabled.

#### `connection.remoteAddress`

The IP address of the connection. For example, '172.217.28.163'.

#### `connection.remoteFamily`

The IP family of the connection. Before the event "connect" or "connection" it's a empty string and after it's always "IPv4".

#### `connection.remotePort`

The remote port. For example, 63750.

## License

MIT
