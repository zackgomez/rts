function SendMessage(msg) {
  var id = msg.to;
  if (!id) {
    Log('Invalid message with recipient id', id);
  }
  var entity = GetEntity(id);
  if (!entity) {
    Log('Unable to find entity', id, 'to send message.');
  }

  entityHandleMessage(entity, msg);
}
