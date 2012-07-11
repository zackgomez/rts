#pragma once
#include <SDL/SDL.h>
#include "Entity.h"
#include <queue>
#include <json/json.h>
#include "glm.h"
#include "PlayerAction.h"

class OpenGLRenderer;
class Game;

class Player
{
public:
    Player(int64_t playerID) : playerID_(playerID), game_(NULL) { }
    virtual ~Player() {}

    int64_t getPlayerID() const { return playerID_; }

    void setGame(Game *game) { game_ = game; }

    // Argument is the tick being simulated
    virtual void update(uint64_t tick) = 0;

protected:
    int64_t playerID_;
    Game *game_;
};

class LocalPlayer : public Player
{
public:
    LocalPlayer(int64_t playerID, OpenGLRenderer *renderer);
    virtual ~LocalPlayer();

    // Called at the start of a tick with the new tick
    virtual void update(uint64_t tick);
    virtual void renderUpdate(float dt);

    // TODO abstract SDL_Even away here
    void handleEvent(const SDL_Event &event);

private:
    void setSelection(eid_t eid);

    OpenGLRenderer *renderer_;
    // The tick the current actions will be executed on
    uint64_t targetTick_; 
    // TODO make an array
    eid_t selection_;
};

