#include "NetPlayer.h"
#include "Game.h"
#include "PlayerAction.h"

NetPlayer::NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock) :
    Player(playerID),
    sock_(sock),
    doneTick_(-1e6)
{
    //netThread_ = std::thread(netThreadFunc, sock_, std::ref(actions_),
            //std::ref(mutex_), std::ref(tickSem_));
}

NetPlayer::~NetPlayer()
{
    // TODO kill thread first
    netThread_.join();
}

bool NetPlayer::update(int64_t tick)
{
    return doneTick_ >= tick;
}

