#include "MessageHub.h"
#include <cassert>
#include "Entity.h"
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

void MessageHub::sendMessage(const Message &msg)
{
    assert (game_);
    eid_t eid = msg["to"].asUInt64();

    game_->sendMessage(eid, msg);
}
