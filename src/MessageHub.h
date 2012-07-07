#pragma once
#include <json/json.h>
#include "Logger.h"

class Game;

typedef Json::Value Message;

class MessageHub
{
public:
    static MessageHub* get();
    ~MessageHub();

    void setGame(Game *game);
    // Immediately dispatches message
    void sendMessage(const Message &msg);
    // TODO dispatchMessage (sends after unit updates unit no more messages)

private:
    // For singleton
    MessageHub();
    static MessageHub *instance;
    Game *game_;
    LoggerPtr logger_;
};

namespace MessageTypes
{
    const std::string ORDER = "ORDER";
};

