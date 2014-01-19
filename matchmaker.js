var net = require('net');
var _ = require('underscore');

var last_id = 100;
var conns = {};

var onNewConnection = function () {
  if (_.size(conns) < 2) {
    return;
  }

  var conn_array = _.toArray(conns);
  conns = {};
  var host = conn_array.shift();
  var client = conn_array.shift();

  var host_message = {
    operation: 'host',
    port: host.remotePort,
  };
  var client_message = {
    operation: 'join',
    host: host.remoteAddress,
    port: host.remotePort,
  };

  host.write(JSON.stringify(host_message));
  client.write(JSON.stringify(client_message));

  host.end();
  client.end();
};

var server = net.createServer(function (c) {
  "use strict";
  var addr_str = c.remoteAddress + ':' + c.remotePort;
  console.log('got connection to', addr_str);
  conns[last_id] = c;
  last_id++;
  c.on('end', function () {
    console.log('closed connection to', addr_str);
    delete conns[last_id];
  });

  onNewConnection();
});

server.listen(11000, function () {
  console.log('started server on port 11000');
});
