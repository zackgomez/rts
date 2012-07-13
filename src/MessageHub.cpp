#include "MessageHub.h"
#include <cassert>
#include "Game.h"

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

const Entity * MessageHub::getEntity(eid_t eid) const
{
    return game_->getEntity(eid);
}

void MessageHub::sendMessage(const Message &msg)
{
    assert (game_);
    eid_t eid = msg["to"].asUInt64();

    if (eid == NO_ENTITY)
        game_->handleMessage(msg);
    else
        game_->sendMessage(eid, msg);
}

