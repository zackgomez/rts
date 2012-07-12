#include "NetPlayer.h"
#include "Game.h"
#include "PlayerAction.h"

void netThread(kissnet::tcp_socket_ptr sock, std::queue<PlayerAction> &queue,
        std::mutex &queueMutex_)
{
    for (;;)
    {
        // TODO deal with exceptions
        // TODO don't assume byte order is the same
        
        // First a 4-byte size header
        ssize_t sz;
        if (sock->recv((char *)&sz, 4) < 4)
            assert(false && "Didn't read a full 4 byte header");

        // Allocate space
        std::string str(sz, '\0');
        if (sock->recv(&str[0], sz) < sz)
            assert(false && "Didn't read a full message");

        std::cout << "Received string: " << str << '\n';
    }
}

NetPlayer::NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock) :
    Player(playerID),
    sock_(sock),
    doneTick_(-1e6),
    localPlayerID_(-1)
{
    logger_ = Logger::getLogger("NetPlayer");
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
    // TODO if its the final sync frame, wait for to be read for first tick
    
    // TODO remove this!
    if (tick < 0)
        return true;
    PlayerAction a;
    a["type"] = ActionTypes::NONE;
    a["tick"] = (Json::Value::Int64) tick;
    a["pid"] = (Json::Value::Int64) playerID_;
    game_->addAction(playerID_, a);
    return true;

    return doneTick_ >= tick;
}

void NetPlayer::playerAction(int64_t playerID, const PlayerAction &action)
{
    // Only send messages relating to the local player(s?) on this machine
    if (playerID == localPlayerID_)
    {
        logger_->info() << "Sending " << action << " across network to player "
            << playerID_ << '\n';
        // TODO queue up the action to be sent, or maybe just send it
    }
}
