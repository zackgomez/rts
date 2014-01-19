var net = require('net');
var _ = require('underscore');

var startServer = function (port) {
  var server = net.createServer(function (c) {
    console.log('Got connection from', c.remoteAddress);
    c.write('hello from server');
    c.on('data', function (data) {
      console.log('client sent', data);
      c.end();
      server.close();
    });
  });
  server.listen(port, function () {
    console.log('started server on port', port);
  });
  server.on('error', function (e) {
    if (e.code == 'EADDRINUSE') {
      console.log('Address in use, retrying...');
      setTimeout(function () {
        server.close();
        server.listen(port);
      }, 100);
    }
  });
};
var connectToServer = function (addr, port) {
  console.log('trying server at', addr, port);
  var conn = net.createConnection({
    host: addr,
    port: port,
  }, function () {
    console.log('connected to', conn.remoteAddress, conn.remotePort);
    conn.write('hello from client');
  });
  conn.on('error', function (error) {
    conn.end();
    console.log('got error', error);
    setTimeout(function () {
      connectToServer(addr, port);
    }, 100);
  });
  conn.on('data', function (data) {
    console.log('server sent', data);
    conn.end();
  });
};

var matchmaker_conn = net.createConnection({
  host: 'zackgomez.com',
  port: 11000,
}, function () {
  console.log('connected to matchmaker');
});
matchmaker_conn.on('data', function (data) {
  matchmaker_conn.end();
  var message = JSON.parse(data);
  console.log('got data', message);
  if (message.operation === 'host') {
    startServer(message.port);
  } else if (message.operation === 'join') {
    connectToServer(message.host, message.port);
  } else {
    throw new Error('unknown operation ' + message.operation);
  }
});
