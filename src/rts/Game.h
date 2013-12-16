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
  float getElapsedTime() const {
    return elapsedTime_;
  }
  bool isRunning() const {
    return running_;
  }
  GameScript* getScript() {
    return &script_;
  }

  // Can possibly block, but should never block long
  void addAction(id_t pid, const PlayerAction &act);

  const GameEntity * getEntity(const std::string &game_id) const;
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }

  float getRequisition(id_t pid) const;
  float getVictoryPoints(id_t tid) const;

  typedef std::function<void(id_t pid, const Json::Value&)> ChatListener;
  void setChatListener(ChatListener cl) {
    chatListener_ = cl;
  }

 private:
  const GameEntity * findEntity(std::function<bool(const GameEntity *)> f) const;
  void updateJS(v8::Handle<v8::Array> player_inputs, float dt);
  // Load the victory points from JS.
  void renderJS();

  v8::Handle<v8::Object> getGameObject();

  std::mutex actionMutex_;
  std::vector<PlayerAction> actions_;

  Map *map_;
  std::vector<Player *> players_;
  // pid => float
  std::map<id_t, float> requisition_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  float elapsedTime_;

  GameScript script_;

  bool running_;

  ChatListener chatListener_;

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
