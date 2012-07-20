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
    assert(msg.isMember("to"));

    Json::Value to = msg["to"];
    
    if (!to.isArray())
    {
        eid_t eid = to.asUInt64();
        if (eid == NO_ENTITY)
            game_->handleMessage(msg);
        else
            game_->sendMessage(eid, msg);
        return;
    }

    // If an array, send to each member
    for (int i = 0; i < to.size(); i++)
        game_->sendMessage(to[i].asInt64(), msg);
}

