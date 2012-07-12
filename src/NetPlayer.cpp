#include "NetPlayer.h"
#include "Game.h"
#include "PlayerAction.h"

void netThreadFunc(kissnet::tcp_socket_ptr sock, std::queue<PlayerAction> &queue,
        std::mutex &queueMutex_)
{
    LoggerPtr logger = Logger::getLogger("NetThread");
    logger->info() << "HELLO!\n";
    for (;;)
    {
        // TODO deal with exceptions
        // TODO don't assume byte order is the same
        
        // First a 4-byte size header
        uint32_t sz;
        ssize_t bytes_read;
        if ((bytes_read = sock->recv((char *)&sz, 4)) < 4)
        {
            if (bytes_read == 0)
                continue;
            logger->fatal() << "Read " << bytes_read << " as header.\n";
            assert(false && "Didn't read a full 4 byte header");
        }

        logger->info() << "Read header of size: " << sz << '\n';

        // Allocate space
        std::string str(sz, '\0');
        if (sock->recv(&str[0], sz) < sz)
            assert(false && "Didn't read a full message");

        std::cout << "Received string: " << str << '\n';

        // TODO queue message, update doneTick if required
    }
}

NetPlayer::NetPlayer(int64_t playerID, kissnet::tcp_socket_ptr sock) :
    Player(playerID),
    sock_(sock),
    doneTick_(-1e6),
    localPlayerID_(-1)
{
    logger_ = Logger::getLogger("NetPlayer");
    netThread_ = std::thread(netThreadFunc, sock_, std::ref(actions_),
            std::ref(mutex_));
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
        std::string body = writer_.write(action);
        uint32_t len = body.size();
        std::string msg((char *) &len, 4);
        msg.append(body);

        //logger_->info() << "Sending " << msg << " across network to player "
            //<< playerID_ << '\n';
        logger_->info() << "Body size " << len << " msg size " << msg.size() << '\n';


        // TODO queue up the action to be sent, or maybe just send it
        sock_->send(msg);
    }
}
