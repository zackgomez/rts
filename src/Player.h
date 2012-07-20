#pragma once
#include <SDL/SDL.h>
#include "Entity.h"
#include <queue>
#include <json/json.h>
#include "glm.h"
#include "PlayerAction.h"
#include "Logger.h"

class OpenGLRenderer;
class Game;

class Player
{
public:
    explicit Player(int64_t playerID) : playerID_(playerID), game_(NULL) { }
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
/* Called each time any player performs an action (including this player).
     * This function should execute quickly (i.e. not perform blocking
     * operations).
     * @arg playerID The player who performed the action.
     * @arg arction The action itself.
     */
    virtual void playerAction(int64_t playerID, const PlayerAction &action) = 0;

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
    virtual void setGame(Game *game);

    virtual void playerAction(int64_t playerID, const PlayerAction &action);

    // Called once per frame with render dt
    virtual void renderUpdate(float dt);

    void quitEvent();
    // TODO(zack) make these events take our own button description, they 
    // currently take SDL_BUTTON_*
    void mouseDown(const glm::vec2 &screenCoord, int button);
    void mouseUp(const glm::vec2 &screenCoord, int button);
    // TODO(zack) create our own keycode representation so we don't have to
    // use SDLs here
    void keyPress(SDLKey key);
    void keyRelease(SDLKey key);

private:
    void setSelection(const std::set<eid_t> &new_selection);

    OpenGLRenderer *renderer_;
    // The tick the current actions will be executed on
    int64_t targetTick_; 
    int64_t doneTick_;
    std::set<eid_t> selection_;

    glm::vec2 cameraPanDir_;

    bool shift_;
    bool leftDrag_;
    glm::vec3 leftStart_;
    std::string order_;

    LoggerPtr logger_;
};


// Player used for testing that occasionally drops frames
class SlowPlayer : public Player
{
public:
    SlowPlayer(int64_t playerID) : Player(playerID) { }

    virtual bool update(int64_t tick);
    virtual void playerAction(int64_t playerID, const PlayerAction &action) { }

private:
};
