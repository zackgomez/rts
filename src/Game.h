#pragma once
#include <glm/glm.hpp>
#include <set>
#include <vector>

class Renderer;
class Map;
class Entity;
class Player;

// Abstract base Game class
class Game
{
public:
    explicit Game(Map *map) : map_(map) { }
    virtual ~Game();

    virtual void update(float dt) = 0;
    virtual void addRenderer(Renderer *renderer);
    virtual const Map * getMap() const { return map_; }

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
    std::vector<Player *> players_;
    std::vector<Entity *> entities_;
};

// TODO rename to ProxyGame a game that gets its information from somewhere else
/*
class LocalGame : public Game
{
public:
    explicit LocalGame(Map *map, Player *player);
    virtual ~LocalGame ();

    virtual void update(float dt);

private:
    Player *localPlayer_;
};
*/

