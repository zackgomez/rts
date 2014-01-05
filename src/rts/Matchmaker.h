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
class Game;

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

  explicit Matchmaker(const Json::Value &player_config);

  // Returns players array, or throws an exception
  Game* waitGame();
  void signalReady(const std::string &mode);
  void signalStop();

  /*
   * Sets up a debug game with a local and dummy player on the 2 player debug
   * map.
   */
  Game* doSinglePlayerSetup();

  typedef std::function<void(const std::string &)> MatchmakerListener;
  
  void registerListener(MatchmakerListener func);

 private:
  Game* connectToServer(
      const std::string &addr,
      const std::string &port);

  Json::Value playerDef_;

  std::string mode_;

  // threading vars
  std::mutex workMutex_;
  std::condition_variable doneCondVar_;
  MatchmakerListener matchmakerStatusCallback_;
};
}  // rts

#endif  // SRC_RTS_MATCHMAKER_H_
