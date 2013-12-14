#ifndef SRC_RTS_GAME_H_
#define SRC_RTS_GAME_H_

#include <set>
#include <vector>
#include <mutex>
#include <queue>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Checksum.h"
#include "common/Logger.h"
#include "rts/GameEntity.h"
#include "rts/GameScript.h"
#include "rts/PlayerAction.h"

namespace rts {

class Map;
class Player;
struct ChatMessage;
struct TickChecksum {
  checksum_t entity_checksum;
  checksum_t action_checksum;
  float random_checksum;

  TickChecksum() {}
  TickChecksum(const Json::Value &val);
  bool operator==(const TickChecksum &rhs) const;
  bool operator!=(const TickChecksum &rhs) const;
  Json::Value toJson() const;
};
class GameRandom;

// Handles the game logic and player actions, is very multithread aware.
class Game {
 public:
  explicit Game(Map *map, const std::vector<Player *> &players);
  ~Game();

  static Game* get() { return instance_; }

  // Synchronizes game between players and does any other initialization
  // required
  void start();

  void update(float dt);
  const Map * getMap() const {
    return map_;
  }
  tick_t getTick() const {
    return tick_;
  }
  float getElapsedTime() const {
    return elapsedTime_;
  }
  bool isPaused() const {
    return paused_;
  }
  bool isRunning() const {
    return running_;
  }
  TickChecksum getChecksum() const {
    return checksums_.back();
  }
  GameScript* getScript() {
    return &script_;
  }

  // Can possibly block, but should never block long
  void addAction(id_t pid, const PlayerAction &act);

  void destroyEntity(id_t eid);
  GameEntity * getEntity(id_t eid);
  const GameEntity * getEntity(const std::string &game_id) const;
  const GameEntity * findEntity(std::function<bool(const GameEntity *)> f) const;
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }

  float getRequisition(id_t pid) const;
  float getVictoryPoints(id_t tid) const;

  typedef std::function<void(id_t pid, const Json::Value&)> ChatListener;
  void setChatListener(ChatListener cl) {
    chatListener_ = cl;
  }

  // Returns a value [0, 1), should be the same across all clients
  float gameRandom();

 private:
  // Returns true if all the players have submitted input for the current tick_
  bool updatePlayers();
  TickChecksum checksum();
  void pause();
  void unpause();
  v8::Handle<v8::Object> getGameObject();

  void updateJS(v8::Handle<v8::Array> player_inputs, float dt);
  // Load the victory points from JS.
  void renderJS();

  std::mutex actionMutex_;

  Map *map_;
  std::vector<Player *> players_;
  // pid => float
  std::map<id_t, float> requisition_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  tick_t tick_;
  tick_t tickOffset_;
  float elapsedTime_;

  GameScript script_;

  // holds checksums before the tick specified by the index
  // checksums_[3] == checksum at the end of tick 2/beginning of tick 3
  std::vector<TickChecksum> checksums_;
  Checksum actionChecksummer_;

  bool paused_;
  bool running_;

  ChatListener chatListener_;
  GameRandom *random_;

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
