// This file holds message related files.  Currently this is just a simple
// convenience function.  Maybe someday it will be more, or be gone.

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
