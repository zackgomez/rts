#pragma once
#include "glm.h"
#include <set>
#include <vector>
#include <mutex>
#include <queue>
#include "PlayerAction.h"
#include "Logger.h"
#include "Entity.h"

namespace rts {

class Renderer;
class Map;
class Player;
struct PlayerResources;

// Handles the game logic and player actions, is very multithread aware.
class Game {
public:
  explicit Game(Map *map, const std::vector<Player *> &players,
      Renderer *renderer);
  ~Game();

  // Synchronizes game between players and does any other initialization
  // required
  // @arg period the time to wait between each check
  void start(float period);

  void update(float dt);
  void render(float dt);
  const Map * getMap() const {
    return map_;
  }
  tick_t getTick() const {
    return tick_;
  }
  // Returns the player action tick delay
  tick_t getTickOffset() const {
    return tickOffset_;
  }
  bool isPaused() const {
    return paused_;
  }
  bool isRunning() const {
    return running_;
  }

  // Does not block, should only be called from Game thread
  void sendMessage(id_t to, const Message &msg);
  void handleMessage(const Message &msg);
  // Can possibly block, but should never block long
  void addAction(id_t pid, const PlayerAction &act);

  const Entity * getEntity(id_t eid) const;
  const Player * getPlayer(id_t pid) const;
  const PlayerResources& getResources(id_t pid) const;

  // Has to be inline, that sucks
  template <class T>
  const Entity * findEntity(T scorer) const {
    float bestscore = HUGE_VAL;
    const Entity *bestentity = NULL;

    for (auto& it : entities_) {
      const Entity *e = it.second;
      float score = scorer(e);
      if (score < bestscore) {
        bestscore = score;
        bestentity = e;
      }
    }

    return bestentity;
  }


protected:
  virtual void handleAction(id_t playerID, const PlayerAction &action);
  // Returns true if all the players have submitted input for the current tick_
  bool updatePlayers();

  LoggerPtr logger_;

  std::mutex mutex_;
  std::mutex actionMutex_;

  Map *map_;
  std::vector<Player *> players_;
  std::map<id_t, Entity *> entities_;
  Renderer *renderer_;
  std::map<id_t, PlayerResources> resources_;
  std::map<id_t, std::queue<PlayerAction>> actions_;
  tick_t tick_;
  tick_t tickOffset_;
  tick_t sync_tick_;

  // Entities that need removal at the end of a tick
  std::vector<id_t> deadEntities_;

  // The messages sent during the last calculated frame
  std::set<Message> messages_;

  bool paused_;
  bool running_;
};

struct PlayerResources {
  float requisition;
};

}; // namespace rts
