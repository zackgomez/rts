#pragma once

#include "Player.h"
#include "kissnet.h"
#include <thread>
#include <mutex>

struct net_msg
{
    uint32_t sz;
    std::string msg;
};

net_msg readPacket(kissnet::tcp_socket_ptr sock)
    throw(kissnet::socket_exception);

class NetPlayer : public Player
{
public:
    NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock);
    virtual ~NetPlayer();

    virtual bool update(int64_t tick);
    virtual void playerAction(int64_t playerID, const PlayerAction &action);

    void setLocalPlayer(int64_t playerID) { localPlayerID_ = playerID; }

private:
    LoggerPtr logger_;
    kissnet::tcp_socket_ptr sock_;
    std::thread netThread_;
    std::queue<PlayerAction> actions_;
    std::mutex mutex_;
    bool running_;
    // last tick that is fully received
    int64_t doneTick_;

    // The id of the local player who's actions we want to send
    int64_t localPlayerID_;

    // JSON writers
    Json::FastWriter writer_;
};

