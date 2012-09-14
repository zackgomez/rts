#ifndef SRC_RTS_MATCHMAKER_H_
#define SRC_RTS_MATCHMAKER_H_

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <json/json.h>
#include "common/Exception.h"
#include "common/NetConnection.h"
#include "common/Types.h"

namespace rts {

class Player;
class LocalPlayer;
class NetPlayer;
class Renderer;

class matchmaker_exception : public exception_with_trace {
 public:
  explicit matchmaker_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

class Matchmaker {
 public:
  Matchmaker(
      const Json::Value &playerConfig,
      Renderer *renderer);

  /*
   * Sets up a debug game with a local and dummy player on the 2 player debug
   * map.
   */
  std::vector<Player *> doDebugSetup();

  /*
   * Sets up a 2 player game with host/client
   * @param ip the IP to connect to, or empty string or hosting
   */
  std::vector<Player *> doDirectSetup(const std::string &ip);

  /*
   * Connects to the matchmaker at the given ip/port and uses that to
   * facilitate player/map discovery/setup.
   */
  std::vector<Player *> doServerSetup(
      const std::string &ip,
      const std::string &port);

  // Returns the name of the map, only valid aver calling a setup method
  std::string getMapName() const;

 private:
  // requires name, color, pid, and tid be set
  void makeLocalPlayer();
  // Throws exception on error
  NetPlayer * handshake(NetConnectionPtr conn) const;
  // Thread worker functions
  void connectToPlayer(const std::string &ip, const std::string &port);
  void acceptConnections(const std::string &port, int numConnections);

  std::string name_;
  glm::vec3 color_;
  std::string listenPort_;

  id_t pid_;
  id_t tid_;

  size_t numPlayers_;

  Renderer *renderer_;

  LocalPlayer *localPlayer_;
  std::vector<Player *> players_;
  std::string mapName_;

  // threading vars
  std::mutex playerMutex_;
  std::condition_variable doneCondVar_;
  bool error_;
};
}  // rts

#endif  // SRC_RTS_MATCHMAKER_H_
