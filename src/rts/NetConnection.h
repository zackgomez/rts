#pragma once
#include <thread>
#include <mutex>
#include "Player.h"
#include "kissnet.h"

class NetConnection {
public:
  NetConnection(kissnet::tcp_socket_ptr sock);
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
