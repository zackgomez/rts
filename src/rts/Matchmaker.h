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

class matchmaker_exception : public exception_with_trace {
 public:
  explicit matchmaker_exception(const std::string &msg) :
    exception_with_trace(msg) {
  }
};

class matchmaker_quit_exception : public std::exception {
};

class Matchmaker {
 public:
  static const std::string MODE_SINGLEPLAYER;
  static const std::string MODE_MATCHMAKING;
  
  explicit Matchmaker(const Json::Value &playerConfig);

  // Returns players array, or throws an exception
  std::vector<Player *> waitPlayers();
  void signalReady(const std::string &mode);
  void signalStop();

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

  id_t getLocalPlayerID() const;
  
  typedef std::function<void(const std::string &)> MatchmakerListener;
  
  void registerListener(MatchmakerListener func);

 private:
  // requires name, color, pid, and tid be set
  void makeLocalPlayer();
  // Throws exception on error
  NetPlayer * handshake(NetConnectionPtr conn) const;

  // Thread worker functions
  void connectToPlayer(const std::string &ip, const std::string &port);
  void acceptConnections(const std::string &port, int numConnections);
  void connectP2P(
      const std::string &publicIP,
      const std::string &privateIP,
      const std::string &connectPort,
      const std::string &listenPort,
      double timeout);

  std::string name_;
  glm::vec3 color_;
  std::string listenPort_;

  std::string mode_;

  id_t pid_;
  id_t tid_;

  size_t numPlayers_;

  LocalPlayer *localPlayer_;
  std::vector<Player *> players_;
  std::string mapName_;

  // threading vars
  std::mutex workMutex_;
  std::mutex playerMutex_;
  std::condition_variable doneCondVar_;
  std::condition_variable playersReadyCondVar_;
  bool error_;
  MatchmakerListener matchmakerStatusCallback_; 
};
}  // rts

#endif  // SRC_RTS_MATCHMAKER_H_
