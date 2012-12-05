#include "common/NetConnection.h"
#include <cassert>
#include "common/Exception.h"
#include "common/Logger.h"
#include "common/util.h"

struct net_msg {
  uint32_t sz;
  std::string msg;
};

// TODO(zack): don't require reading header/payload in one shot
net_msg readPacket(kissnet::tcp_socket_ptr sock, double timeout)
    throw (kissnet::socket_exception) {
  net_msg ret;

  kissnet::socket_set set;
  set.add_read_socket(sock);

  auto socks = set.poll_sockets(timeout);
  if (socks.empty()) {
    ret.sz = 0;
    return ret;
  }

  // TODO(zack): check for socket error

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

void netThreadFunc(
    kissnet::tcp_socket_ptr sock,
    std::queue<Json::Value> &queue,
    std::mutex &queueMutex,
    std::condition_variable &condVar,
    bool &running) {

  sock->setNonBlocking();

  Json::Reader reader;
  while (running) {
    Json::Value msg;
    try {
      // TODO(zack): add maybe 100ms timeout to this so the game/thread joins
      // more readily
      net_msg packet = readPacket(sock, 0.1);

      // It was a timeout, just try again
      if (packet.sz == 0) {
        continue;
      }

      // Parse
      reader.parse(packet.msg, msg);
    } catch (kissnet::socket_exception e) {
      LOG(ERROR) << "Caught socket exception '" << e.what()
                      << "'... terminating thread.\n";
      // On exception, quit thread
      running = false;
    }

    // Lock and queue
    std::unique_lock<std::mutex> lock(queueMutex);
    if (running) {
      queue.push(msg);
    }
    // Wake waiting thread, if applicable
    condVar.notify_one();
    // automatically unlocks when lock goes out of scope
  }

  LOG(INFO) << "Thread finished\n";
}

NetConnection::NetConnection(kissnet::tcp_socket_ptr sock)
  : running_(true),
  sock_(sock) {
  netThread_ = std::thread(
    netThreadFunc,
    sock_,
    std::ref(queue_),
    std::ref(mutex_),
    std::ref(condVar_),
    std::ref(running_));
}

NetConnection::~NetConnection() {
  stop();
  netThread_.join();
}

void NetConnection::stop() {
  running_ = false;
}

void NetConnection::sendPacket(const Json::Value &message) {
  Json::FastWriter writer;
  std::string body = writer.write(message);
  uint32_t len = body.size();
  // TODO(zack) endianness issue here?
  std::string msg((char *) &len, 4);
  msg.append(body);

  // Just send out the message
  sock_->send(msg);
}

Json::Value NetConnection::readNext() {
  std::unique_lock<std::mutex> lock(mutex_);
  // Wait until queue has value, or thread stopped
  condVar_.wait(lock, [this]() {return !running_ || !queue_.empty();});

  // if there is a message, lets return it first
  if (!running_ && queue_.empty()) {
    throw network_exception("network thread died");
  }

  invariant(!queue_.empty(), "queue shouldn't be empty");
  Json::Value ret = queue_.front();
  queue_.pop();

  // Lock automatically goes out of scope
  return ret;
}

Json::Value NetConnection::readNext(size_t millis) {
  std::unique_lock<std::mutex> lock(mutex_);
  LOG(INFO) << "timeout is " << millis << " ms.\n";
  // Wait until queue has value, thread stopped, or timeout
  bool success = condVar_.wait_for(
    lock,
    std::chrono::milliseconds(millis),
    [this]() {return !running_ || !queue_.empty();});

  if (!success) {
    throw timeout_exception();
  }
  if (!running_ && queue_.empty()) {
    throw network_exception("network thread died");
  }

  invariant(!queue_.empty(), "queue should not be empty!");
  Json::Value ret = queue_.front();
  queue_.pop();

  // Lock automatically goes out of scope
  return ret;
}
