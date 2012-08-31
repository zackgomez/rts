#ifndef SRC_COMMON_NETCONNECTION_H_
#define SRC_COMMON_NETCONNECTION_H_
#include <thread>
#include <mutex>
#include <queue>
#include <json/json.h>
#include "kissnet.h"

class NetConnection {
 public:
  explicit NetConnection(kissnet::tcp_socket_ptr sock);
  ~NetConnection();

  std::queue<Json::Value>& getQueue() {
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

  void sendPacket(const Json::Value &msg);

 private:
  bool running_;
  kissnet::tcp_socket_ptr sock_;
  std::queue<Json::Value> queue_;
  std::mutex mutex_;
  std::thread netThread_;

  // JSON writers
  Json::FastWriter writer_;
};

#endif  // SRC_COMMON_NETCONNECTION_H_
