var invariant = require('invariant').invariant;

// id -> [messages]
var message_queue = {};

exports.sendMessage = function (msg) {
  invariant('to' in msg, 'messages must have to param');
  if (!(msg.to in message_queue)) {
    message_queue[msg.to] = [];
  }
  message_queue[msg.to].push(msg);
};

exports.getMessagesForEntity = function (id) {
  return message_queue[id] || [];
}

exports.clearMessages = function () {
  message_queue = {};
}
