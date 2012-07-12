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

    virtual void setGame(Game *game) { game_ = game; }

    /* Called at the start of each tick, should finalize the previous frame
     * and begin preparing input for the next frame.
     * NOTE: this may be called multiple times per tick, if some players aren't
     * ready.
     *
     * @arg tick is the tick being simulated
     * @return true, if this player has submitted all input for the given frame
     */
    virtual bool update(int64_t tick) = 0;

protected:
    int64_t playerID_;
    Game *game_;
};

class LocalPlayer : public Player
{
public:
    LocalPlayer(int64_t playerID, OpenGLRenderer *renderer);
    virtual ~LocalPlayer();

    virtual bool update(int64_t tick);
    virtual void renderUpdate(float dt);
    virtual void setGame(Game *game);

    // TODO abstract SDL_Even away here
    void handleEvent(const SDL_Event &event);

private:
    void setSelection(eid_t eid);

    OpenGLRenderer *renderer_;
    // The tick the current actions will be executed on
    int64_t targetTick_; 
    int64_t doneTick_;
    // TODO make an array
    eid_t selection_;

    LoggerPtr logger_;
};


// Player used for testing that occasionally drops frames
class SlowPlayer : public Player
{
public:
    SlowPlayer(int64_t playerID) : Player(playerID) { }

    virtual bool update(int64_t tick);

private:
};
