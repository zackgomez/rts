#include "NetConnection.h"

struct net_msg {
  uint32_t sz;
  std::string msg;
};

// TODO(zack): don't require reading header/payload in one shot
net_msg readPacket(kissnet::tcp_socket_ptr sock)
throw(kissnet::socket_exception) {
  net_msg ret;

  int bytes_read;
  // First read header
  if ((bytes_read = sock->recv((char *)&ret.sz, 4)) < 4) {
    if (bytes_read == 0) {
      throw kissnet::socket_exception("client disconnected gracefully");
    }

    Logger::getLogger("readPacket")->fatal() << "Read " << bytes_read
        << " as header.\n";
    assert(false && "Didn't read a full 4 byte header");
  }

  // TODO(zack) don't assume byte order is the same

  // Then allocate and read payload
  ret.msg = std::string(ret.sz, '\0');
  if (sock->recv(&ret.msg[0], ret.sz) < ret.sz) {
    assert(false && "Didn't read a full message");
  }

  return ret;
}

void netThreadFunc(kissnet::tcp_socket_ptr sock, std::queue<PlayerAction> &queue,
                   std::mutex &queueMutex, bool &running) {
  LoggerPtr logger = Logger::getLogger("NetThread");
  Json::Reader reader;
  while (running) {
    // Each loop, push exactly one message onto queue
    PlayerAction act;
    try {
      net_msg packet = readPacket(sock);

      // Parse
      reader.parse(packet.msg, act);

      //logger->debug() << "Received action: " << act << '\n';

    } catch (kissnet::socket_exception e) {
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
  sock_(sock) {
  netThread_ = std::thread(netThreadFunc, sock_, std::ref(queue_),
                           std::ref(mutex_), std::ref(running_));
}

NetConnection::~NetConnection() {
  stop();
  netThread_.join();
}

void NetConnection::stop() {
  running_ = false;
}

void NetConnection::sendPacket(const Json::Value &message) {
  std::string body = writer_.write(message);
  uint32_t len = body.size();
  // TODO(zack) endianness issue here?
  std::string msg((char *) &len, 4);
  msg.append(body);

  // Just send out the message
  sock_->send(msg);
}
