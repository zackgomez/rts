#include "MessageHub.h"
#include <cassert>
#include "Game.h"

namespace rts {

MessageHub * MessageHub::instance = NULL;

MessageHub * MessageHub::get()
{
  if (!instance)
    instance = new MessageHub();
  return instance;
}

MessageHub::MessageHub()
  : game_(NULL)
{
  logger_ = Logger::getLogger("MessageHub");
}

MessageHub::~MessageHub()
{
}

void MessageHub::setGame(Game *game)
{
  game_ = game;
}

const Entity * MessageHub::getEntity(id_t eid) const
{
  return game_->getEntity(eid);
}

void MessageHub::sendMessage(const Message &msg)
{
  assert (game_);
  assert(msg.isMember("to"));

  Json::Value to = msg["to"];

  if (!to.isArray())
  {
    id_t eid = toID(to);
    if (eid == NO_ENTITY)
      game_->handleMessage(msg);
    else
      game_->sendMessage(eid, msg);
    return;
  }

  // If an array, send to each member
  for (int i = 0; i < to.size(); i++)
    game_->sendMessage(toID(to[i]), msg);
}

void MessageHub::sendRemovalMessage(const Entity *e)
{
	assert(e);

	Message msg;
	msg["to"] = toJson(GAME_ID);
	msg["type"] = MessageTypes::DESTROY_ENTITY;
	msg["eid"] = toJson(e->getID());

	sendMessage(msg);
}

}; // rts
