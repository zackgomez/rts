#pragma once
#include <json/json.h>
#include "Logger.h"
#include "Entity.h"
#include "Message.h"

class Game;

class MessageHub
{
public:
    static MessageHub* get();
    ~MessageHub();

    void setGame(Game *game);

    const Entity *getEntity(eid_t eid) const;
    // Immediately dispatches message
    void sendMessage(const Message &msg);
    // TODO dispatchMessage (sends after unit updates until no more messages)

private:
    // For singleton
    MessageHub();
    static MessageHub *instance;
    Game *game_;
    LoggerPtr logger_;
};
