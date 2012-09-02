#include "common/NetConnection.h"
#include <cassert>
#include "common/Logger.h"

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
    LOG(FATAL) << "Read " << bytes_read << " as header.\n";
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

void netThreadFunc(kissnet::tcp_socket_ptr sock, std::queue<Json::Value> *queue,
                   std::mutex *queueMutex, bool *running) {
  Json::Reader reader;
  while (*running) {
    // Each loop, push exactly one message onto queue
    Json::Value msg;
    try {
      // TODO(zack): add maybe 100ms timeout to this so the game/thread joins
      // more readily
      net_msg packet = readPacket(sock);

      // Parse
      reader.parse(packet.msg, msg);
    } catch (kissnet::socket_exception e) {
      LOG(ERROR) << "Caught socket exception '" << e.what()
                      << "'... terminating thread.\n";
      // On exception, quit thread
      *running = false;
    }

    // Lock and queue
    std::unique_lock<std::mutex> lock(*queueMutex);
    queue->push(msg);
    // automatically unlocks when lock goes out of scope
  }

  LOG(INFO) << "Thread finished\n";
}

NetConnection::NetConnection(kissnet::tcp_socket_ptr sock)
  : running_(true),
  sock_(sock) {
  netThread_ = std::thread(netThreadFunc, sock_, &queue_, &mutex_, &running_);
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
