#ifndef SRC_RTS_GAME_H_
#define SRC_RTS_GAME_H_

#include <glm/glm.hpp>
#include <set>
#include <vector>
#include <mutex>
#include <queue>
#include "common/Clock.h"
#include "common/Checksum.h"
#include "common/Logger.h"
#include "rts/GameEntity.h"
#include "rts/PlayerAction.h"

namespace rts {

class Map;
class Player;
struct PlayerResources {
  float requisition;
};
struct ChatMessage;

// Handles the game logic and player actions, is very multithread aware.
class Game {
 public:
  explicit Game(Map *map, const std::vector<Player *> &players);
  ~Game();

  static const Game* get() { return instance_; }

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
  checksum_t getChecksum() const {
    return checksums_.back();
  }
  const std::vector<ChatMessage>& getChatMessages() const {
    return chats_;
  }

  // Does not block, should only be called from Game thread
  void sendMessage(id_t to, const Message &msg);
  void handleMessage(const Message &msg);
  // Can possibly block, but should never block long
  void addAction(id_t pid, const PlayerAction &act);

  const GameEntity * getEntity(id_t eid) const;
  const std::map<id_t, GameEntity *>& getEntities() const { return entities_; }
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }
  const std::set<id_t> getTeams() const { return teams_; }

  const PlayerResources& getResources(id_t pid) const;
  float getVictoryPoints(id_t tid) const;

  // Returns the entity that scores the LOWEST with the given scoring function.
  // Scoring function should have signature float scorer(const GameEntity *);
  template <class T>
  const GameEntity * findEntity(T scorer) const {
    float bestscore = HUGE_VAL;
    const GameEntity *bestentity = nullptr;

    for (const auto& it : entities_) {
      const GameEntity *e = it.second;
      float score = scorer(e);
      if (score < bestscore) {
        bestscore = score;
        bestentity = e;
      }
    }

    return bestentity;
  }


 protected:
  void handleAction(id_t playerID, const PlayerAction &action);
  // Returns true if all the players have submitted input for the current tick_
  bool updatePlayers();
  checksum_t checksum();
  void pause();
  void unpause();

  std::mutex actionMutex_;

  Map *map_;
  std::vector<Player *> players_;
  std::map<id_t, GameEntity *> entities_;
  std::set<id_t> teams_;
  // pid => float
  std::map<id_t, PlayerResources> resources_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  tick_t tick_;
  tick_t tickOffset_;

  std::vector<ChatMessage> chats_;

  // holds checksums before the tick specified by the index
  // checksums_[3] == checksum at the end of tick 2/beginning of tick 3
  std::vector<checksum_t> checksums_;

  // Entities that need removal at the end of a tick
  std::vector<id_t> deadEntities_;

  bool paused_;
  bool running_;

 private:
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
