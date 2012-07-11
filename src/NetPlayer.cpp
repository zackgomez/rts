#include "NetPlayer.h"
#include "Game.h"
#include "PlayerAction.h"

NetPlayer::NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock) :
    Player(playerID),
    sock_(sock)
{
    //netThread_ = std::thread(netThreadFunc, sock_, std::ref(actions_),
            //std::ref(mutex_), std::ref(tickSem_));
}

NetPlayer::~NetPlayer()
{
    // TODO kill thread first
    netThread_.join();
}

void NetPlayer::update(uint64_t tick)
{
}

