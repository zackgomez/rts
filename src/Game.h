#pragma once
#include <glm/glm.hpp>
#include <set>
#include <vector>

class Renderer;
class Map;
class Entity;
class Player;
class PlayerAction;

// Abstract base Game class
class Game
{
public:
    explicit Game(Map *map) : map_(map) { }
    virtual ~Game();

    virtual void update(float dt) = 0;
    virtual void addRenderer(Renderer *renderer);
    virtual const Map * getMap() const { return map_; }

    //virtual const Entity * getEntity() const = 0;

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

protected:
    virtual void handleAction(int64_t playerID, const PlayerAction &action);

    std::vector<Player *> players_;
    std::vector<Entity *> entities_;
};

