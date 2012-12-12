#ifndef SRC_RTS_PLAYER_H_
#define SRC_RTS_PLAYER_H_

#include <set>
#include <string>
#include <queue>
#include <mutex>
#include <glm/glm.hpp>
#include <json/json.h>
#include <SDL/SDL.h>
#include "common/Logger.h"
#include "common/Types.h"
#include "rts/PlayerAction.h"

namespace rts {

class Game;

namespace PlayerState {
const std::string DEFAULT = "DEFAULT";
const std::string CHATTING = "CHATTING";
}

class Player {
 public:
  explicit Player(id_t playerID, id_t teamID, const std::string &name,
                  const glm::vec3 &color)
      : playerID_(playerID), teamID_(teamID), name_(name), color_(color),
        game_(nullptr) {
    assertPid(playerID_);
    assertTid(teamID_);
  }
  virtual ~Player() {}

  void setGame(Game *game) {
    game_ = game;
  }

  id_t getPlayerID() const {
    return playerID_;
  }

  id_t getTeamID() const {
    return teamID_;
  }

  const std::string &getName() const {
    return name_;
  }

  virtual void startTick(tick_t tick) = 0;

  // Returns true if the player has the actions for the current tick.
  // May be called multiple times per tick
  virtual bool isReady() const = 0;

  /*
   * Called when all players are ready, returns the actions to be executed.
   *
   * @return list of player actions for the current tick.  Will always
   * contain at least a DONE or LEAVE_GAME action
   */
  virtual std::vector<PlayerAction> getActions() = 0;

  /* Called each time any player performs an action (including this player).
   * This function should execute quickly (i.e. not perform blocking
   * operations).
   * @param playerID The player who performed the action.
   * @param action The action itself.
   */
  virtual void playerAction(id_t playerID, const PlayerAction &action) = 0;

  /* Returns this player's color. */
  glm::vec3 getColor() const {
    return color_;
  }

 protected:
  id_t playerID_;
  id_t teamID_;
  const std::string name_;
  glm::vec3 color_;

  Game *game_;
};

class LocalPlayer : public Player {
 public:
  LocalPlayer(id_t playerID, id_t teamID, const std::string &name,
              const glm::vec3 &color);
  virtual ~LocalPlayer();

  virtual std::vector<PlayerAction> getActions();
  virtual void setGame(Game *game);

  virtual void playerAction(id_t playerID, const PlayerAction &action);

  virtual void startTick(tick_t tick);
  virtual bool isReady() const;
  // Called once per frame with render dt
  virtual void renderUpdate(float dt);

  void quitEvent();
  // TODO(zack) make these events take our own button description, they
  // currently take SDL_BUTTON_*
  void mouseDown(const glm::vec2 &screenCoord, int button);
  void mouseUp(const glm::vec2 &screenCoord, int button);
  // TODO(zack) create our own keycode representation so we don't have to
  // use SDLs here
  void keyPress(SDL_keysym key);
  void keyRelease(SDL_keysym key);

  // Returns the message the player is currently typing
  const std::string& getLocalMessage() const { return message_; }

  const std::string& getState() const { return state_; }

 private:
  void setSelection(const std::set<id_t> &new_selection);
  void addAction(const PlayerAction &a);

  std::queue<PlayerAction> actions_;
  std::mutex actionMutex_;

  std::set<id_t> selection_;

  glm::vec2 cameraPanDir_;

  bool shift_;
  bool chatting_;
  std::string message_;
  bool leftDrag_;
  glm::vec3 leftStart_;
  bool leftDragMinimap_;
  std::string order_;
  std::string state_;

  LoggerPtr logger_;
};

class DummyPlayer : public Player {
 public:
  DummyPlayer(id_t playerID, id_t teamID);
  virtual std::vector<PlayerAction> getActions();
  virtual void startTick(tick_t tick);
  virtual bool isReady() const { return true; }
  virtual void playerAction(id_t playerID, const PlayerAction &action) { }

 private:
  std::queue<PlayerAction> actions_;
};
};  // rts

#endif  // SRC_RTS_PLAYER_H_
