#pragma once
#include <thread>
#include <mutex>
#include "Player.h"
#include "kissnet.h"

namespace rts {

struct net_msg {
  uint32_t sz;
  std::string msg;
};

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

class NetPlayer : public Player {
public:
  NetPlayer(id_t playerID, id_t teamID, const std::string &name,
      const glm::vec3 &color, NetConnection *conn);
  virtual ~NetPlayer();

  virtual bool update(tick_t tick);
  virtual void playerAction(id_t playerID, const PlayerAction &action);

  void setLocalPlayer(id_t playerID) {
    localPlayerID_ = playerID;
  }

private:
  LoggerPtr logger_;
  NetConnection *connection_;
  std::mutex mutex_;
  // last tick that is fully received
  tick_t doneTick_;

  // The id of the local player who's actions we want to send
  id_t localPlayerID_;
};

}; // rts
