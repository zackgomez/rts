#ifndef SRC_RTS_GAME_H_
#define SRC_RTS_GAME_H_

#include <vector>
#include <mutex>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Logger.h"
#include "rts/GameScript.h"

namespace rts {

class Map;
class Player;
class GameEntity;
struct ChatMessage;

class Game {
 public:
  explicit Game(Map *map, const std::vector<Player *> &players);
  ~Game();

  static Game* get() { return nullthrows(instance_); }

  void renderFromJSON(const Json::Value&);

  const Map * getMap() const {
    return map_;
  }
  float getElapsedTime() const {
    return elapsedTime_;
  }
  bool isRunning() const {
    return running_;
  }

  const GameEntity * getEntity(const std::string &game_id) const;
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }

  float getRequisition(id_t pid) const;
  float getVictoryPoints(id_t tid) const;

 private:
  std::map<std::string, id_t> game_to_render_id;

  bool running_;

  Map *map_;
  std::vector<Player *> players_;
  // pid => float
  std::map<id_t, float> requisition_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  float elapsedTime_;

  static Game *instance_;
};

struct ChatMessage {
  ChatMessage(const std::string &m, const Clock::time_point &t)
      : msg(m), time(t) { }

  std::string msg;
  Clock::time_point time;
};
};  // namespace rts

#endif  // SRC_RTS_GAME_H_
