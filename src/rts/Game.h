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
class VisibilityMap;
struct PlayerResources {
  float requisition;
};
struct ChatMessage;
struct TickChecksum {
  checksum_t entity_checksum;
  checksum_t action_checksum;

  TickChecksum() {}
  TickChecksum(const Json::Value &val);
  bool operator==(const TickChecksum &rhs) const;
  bool operator!=(const TickChecksum &rhs) const;
  Json::Value toJson() const;
};

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

  void addResources(
      id_t pid,
      ResourceType type,
      float amount,
      id_t from);
  void addVPs(id_t tid, float amount, id_t from);

  const GameEntity * spawnEntity(
      const std::string &name,
      const Json::Value &params);
  void destroyEntity(id_t eid);
  GameEntity * getEntity(id_t eid);
  const GameEntity * getEntity(id_t eid) const;
  const GameEntity * findEntity(std::function<bool(const GameEntity *)> f) const;
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }
  const std::set<id_t> getTeams() const { return teams_; }

  const PlayerResources& getResources(id_t pid) const;
  float getVictoryPoints(id_t tid) const;
  const VisibilityMap* getVisibilityMap(id_t tid) const;

  typedef std::function<void(id_t pid, const Message&)> ChatListener;
  void setChatListener(ChatListener cl) {
    chatListener_ = cl;
  }

 private:
  void initScripts();
  void handleOrder(id_t playerID, const PlayerAction &action);
  // Returns true if all the players have submitted input for the current tick_
  bool updatePlayers();
  // This just updates JS player info, game data related rather than action related
  void updateJSPlayers();
  TickChecksum checksum();
  void pause();
  void unpause();

  std::mutex actionMutex_;

  Map *map_;
  std::vector<Player *> players_;
  std::set<id_t> teams_;
  // pid => float
  std::map<id_t, PlayerResources> resources_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  tick_t tick_;
  tick_t tickOffset_;

  GameScript script_;

  std::map<id_t, VisibilityMap*> visibilityMaps_;

  // holds checksums before the tick specified by the index
  // checksums_[3] == checksum at the end of tick 2/beginning of tick 3
  std::vector<TickChecksum> checksums_;
  Checksum actionChecksummer_;

  // Entities that need removal at the end of a tick
  std::set<id_t> deadEntities_;

  bool paused_;
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
