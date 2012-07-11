#pragma once

#include "Player.h"
#include "kissnet.h"
#include <thread>
#include <mutex>
#include "Semaphore.h"

class NetPlayer : public Player
{
public:
    NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock);
    virtual ~NetPlayer();

    virtual void update(uint64_t tick);

private:
    kissnet::tcp_socket_ptr sock_;
    std::thread netThread_;
    std::mutex mutex_;
};

