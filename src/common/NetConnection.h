#ifndef SRC_COMMON_NETCONNECTION_H_
#define SRC_COMMON_NETCONNECTION_H_

#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <json/json.h>
#include "common/kissnet.h"

class NetConnection {
 public:
  explicit NetConnection(kissnet::tcp_socket_ptr sock);
  ~NetConnection();

  std::vector<Json::Value> drainQueue() {
    std::unique_lock<std::mutex>(mutex_);
    std::vector<Json::Value> ret;
    ret.swap(queue_);
    return ret;
  }

  std::vector<Json::Value>& getQueue() {
    return queue_;
  }

  kissnet::tcp_socket_ptr getSocket() {
    return sock_;
  }

  std::mutex& getMutex() {
    return mutex_;
  }

  void stop();

  bool running() const {
    return running_;
  }

  // Blocks until next message becomes available.
  // throws an exception if the socket is closed unexpectedly
  // throws an exception on timeout (arg version)
  Json::Value readNext();
  Json::Value readNext(size_t millis);

  void sendPacket(const Json::Value &msg);

 private:
  bool running_;
  kissnet::tcp_socket_ptr sock_;
  std::vector<Json::Value> queue_;
  std::mutex mutex_;
  std::condition_variable condVar_;
  std::thread netThread_;
};

typedef std::shared_ptr<NetConnection> NetConnectionPtr;

#endif  // SRC_COMMON_NETCONNECTION_H_
