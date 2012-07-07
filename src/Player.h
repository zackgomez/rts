#pragma once
#include <glm/glm.hpp>
#include <SDL/SDL.h>
#include "Entity.h"
#include <queue>

struct PlayerAction;
class OpenGLRenderer;

class Player
{
public:
    Player(int64_t playerID) : playerID_(playerID) { }
    virtual ~Player() {}

    int64_t getPlayerID() const { return playerID_; }

    virtual void update(float dt) = 0;
    virtual PlayerAction getAction() = 0;

protected:
    int64_t playerID_;
};

struct PlayerAction
{
    enum ActionType {
        NONE = 0,
    };

    ActionType type;
};

class LocalPlayer : public Player
{
public:
    LocalPlayer(int64_t playerID, OpenGLRenderer *renderer);
    virtual ~LocalPlayer();

    virtual void update(float dt);
    virtual PlayerAction getAction();

    virtual void handleEvent(const SDL_Event &event);

private:
    OpenGLRenderer *renderer_;

    void setSelection(eid_t eid);

    eid_t selection_;
    std::queue<PlayerAction> actions_;
};
