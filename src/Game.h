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

    // Synchronizes game between players and does any other initialization
    // required
    // @arg period the time to wait between each check
    void start(float period);

    void update(float dt);
    void render(float dt);
    void addRenderer(Renderer *renderer);
    const Map * getMap() const { return map_; }
    int64_t getTick() const { return tick_; }
    // Returns the player action tick delay
    int64_t getTickOffset() const { return tickOffset_; }
    bool isPaused() const { return paused_; }
    bool isRunning() const { return running_; }

    // Does not block, should only be called from Game thread
    void sendMessage(eid_t to, const Message &msg);
    void handleMessage(const Message &msg);
    // Can possibly block, but should never block long
    void addAction(int64_t pid, const PlayerAction &act);

    const Entity * getEntity(eid_t eid) const;


protected:
    const Player * getPlayer(int64_t pid) const;
    virtual void handleAction(int64_t playerID, const PlayerAction &action);
    // Returns true if all the players have submitted input for the current tick_
    bool updatePlayers();

    LoggerPtr logger_;

    std::mutex mutex_;
    std::mutex actionMutex_;

    Map *map_;
    std::vector<Player *> players_;
    std::map<eid_t, Entity *> entities_;
    std::set<Renderer *> renderers_;
    std::map<int64_t, std::queue<PlayerAction>> actions_;
    int64_t tick_;
    int64_t tickOffset_;
    int64_t sync_tick_;

    bool paused_;
    bool running_;
};

