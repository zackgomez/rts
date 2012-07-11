#pragma once
#include "glm.h"
#include <set>
#include <vector>
#include <mutex>
#include <queue>
#include "PlayerAction.h"
#include "Logger.h"
#include "Entity.h"

class Renderer;
class Map;
class Player;

// Handles the game logic and player actions, is very multithread aware.
class Game
{
public:
    explicit Game(Map *map, const std::vector<Player *> &players);
    ~Game();

    void update(float dt);
    void render(float dt);
    void addRenderer(Renderer *renderer);
    const Map * getMap() const { return map_; }
    uint64_t getTick() const { return tick_; }
    uint64_t getTickOffset() const { return tickOffset_; }

    // Does not block, should only be called from Game thread
    void sendMessage(eid_t to, const Message &msg);
    // Can possibly block, but should never block long
    void addAction(int64_t pid, const PlayerAction &act);

    const Entity * getEntity(eid_t eid) const;
    const Player * getPlayer(int64_t pid) const;


protected:
    virtual void handleAction(int64_t playerID, const PlayerAction &action);

    LoggerPtr logger_;

    std::mutex mutex_;
    std::mutex actionMutex_;

    Map *map_;
    std::vector<Player *> players_;
    std::map<eid_t, Entity *> entities_;
    std::set<Renderer *> renderers_;
    std::map<int64_t, std::queue<PlayerAction>> actions_;
    uint64_t tick_;
    uint64_t tickOffset_;
};

