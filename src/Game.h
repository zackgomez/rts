#pragma once
#include "glm.h"
#include <set>
#include <vector>
#include "PlayerAction.h"
#include "Logger.h"
#include "Entity.h"

class Renderer;
class Map;
class Player;

// Abstract base Game class
class Game
{
public:
    explicit Game(Map *map) : map_(map) { }
    virtual ~Game();

    virtual void update(float dt) = 0;
    virtual void render(float dt) = 0;
    virtual void addRenderer(Renderer *renderer);
    virtual const Map * getMap() const { return map_; }

    virtual void sendMessage(eid_t to, const Message &msg) = 0;

    virtual const Entity * getEntity(eid_t eid) const = 0;
    virtual const Player * getPlayer(int64_t pid) const = 0;

protected:
    std::set<Renderer *> renderers_;
    Map *map_;
};

// Actually runs the game logic
class HostGame : public Game
{
public:
    explicit HostGame(Map *map, const std::vector<Player *> &players);
    virtual ~HostGame();

    virtual void update(float dt);
    virtual void render(float dt);
    virtual void sendMessage(eid_t to, const Message &msg);

    virtual const Entity * getEntity(eid_t eid) const;
    virtual const Player * getPlayer(int64_t pid) const;

protected:
    virtual void handleAction(int64_t playerID, const PlayerAction &action);

    LoggerPtr logger_;

    std::vector<Player *> players_;
    std::map<eid_t, Entity *> entities_;
};

