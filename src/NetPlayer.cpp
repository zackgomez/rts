#include "NetPlayer.h"
#include "Game.h"
#include "PlayerAction.h"
#include "util.h"

namespace rts {

// TODO(zack): don't require reading header/payload in one shot
net_msg readPacket(kissnet::tcp_socket_ptr sock)
  throw(kissnet::socket_exception)
  {
    net_msg ret;

    int bytes_read;
    // First read header
    if ((bytes_read = sock->recv((char *)&ret.sz, 4)) < 4)
    {
      if (bytes_read == 0)
        throw kissnet::socket_exception("client disconnected gracefully");

      Logger::getLogger("readPacket")->fatal() << "Read " << bytes_read
        << " as header.\n";
      assert(false && "Didn't read a full 4 byte header");
    }

    // TODO(zack) don't assume byte order is the same

    // Then allocate and read payload
    ret.msg = std::string(ret.sz, '\0');
    if (sock->recv(&ret.msg[0], ret.sz) < ret.sz)
      assert(false && "Didn't read a full message");

    return ret;
  }

void netThreadFunc(kissnet::tcp_socket_ptr sock, std::queue<PlayerAction> &queue,
    std::mutex &queueMutex, bool &running)
{
  LoggerPtr logger = Logger::getLogger("NetThread");
  Json::Reader reader;
  while (running)
  {
    // Each loop, push exactly one message onto queue
    PlayerAction act;
    try
    {
      net_msg packet = readPacket(sock);

      // Parse
      reader.parse(packet.msg, act);

      //logger->debug() << "Received action: " << act << '\n';

    }
    catch (kissnet::socket_exception e)
    {
      logger->error() << "Caught socket exception '" << e.what() 
        << "'... terminating thread.\n";
      // On exception, leave game
      act["type"] = ActionTypes::LEAVE_GAME;
      running = false;
    }

    // Lock and queue
    std::unique_lock<std::mutex> lock(queueMutex);
    queue.push(act);
    // automatically unlocks when lock goes out of scope
  }

  logger->info() << "Thread finished\n";
}

NetConnection::NetConnection(kissnet::tcp_socket_ptr sock) :
  running_(true),
  sock_(sock)
{
  netThread_ = std::thread(netThreadFunc, sock_, std::ref(queue_),
      std::ref(mutex_), std::ref(running_));
}

NetConnection::~NetConnection()
{
  stop();
  netThread_.join();
}

void NetConnection::stop()
{
  running_ = false;
}

void NetConnection::sendPacket(const Json::Value &message)
{
  std::string body = writer_.write(message);
  uint32_t len = body.size();
  // TODO(zack) endianness issue here?
  std::string msg((char *) &len, 4);
  msg.append(body);

  // Just send out the message
  sock_->send(msg);
}

NetPlayer::NetPlayer(int64_t playerID, NetConnection *conn) :
  Player(playerID),
  connection_(conn),
  doneTick_(-1e6),
  localPlayerID_(-1)
{
  assert(connection_);
  logger_ = Logger::getLogger("NetPlayer");
}

NetPlayer::~NetPlayer()
{
  connection_->stop();
  delete connection_;
}

bool NetPlayer::update(tick_t tick)
{
  std::unique_lock<std::mutex> lock(connection_->getMutex());
  std::queue<PlayerAction> &actions = connection_->getQueue();
  // Read as many actions as were queued
  while (!actions.empty())
  {
    PlayerAction a = actions.front();
    actions.pop();

    invariant(
      a["pid"].asInt64() == playerID_ || a["type"] == ActionTypes::LEAVE_GAME,
      "bad action from network thread"
    );
    game_->addAction(playerID_, a);

    // if we get a none, then we're done with another frame... make it so
    if (a["type"] == ActionTypes::DONE)
    {
      invariant(a["tick"].asInt64() > doneTick_, "bad tick in action");
      doneTick_ = (int64_t) a["tick"].asInt64();
    }

    // Messages generated by the net thread don't know our pid, fill it in
    if (!a.isMember("pid"))
      a["pid"] = toJson(playerID_);
  }

  // Have we received all the messages for this tick?
  return doneTick_ >= tick;
}

void NetPlayer::playerAction(id_t playerID, const PlayerAction &action)
{
  // Ignore our own messages
  if (playerID == playerID_)
    return;

  // Only send messages relating to the local player(s?) on this machine
  if (playerID == localPlayerID_)
    connection_->sendPacket(action);

  // If the action was a leave game, close down thread
  if (action["type"] == ActionTypes::LEAVE_GAME)
    connection_->stop();
}

}; // rts
