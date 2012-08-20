#pragma once
#include <queue>
#include <SDL/SDL.h>
#include <json/json.h>
#include <glm/glm.hpp>
#include <string>
#include "PlayerAction.h"
#include "Entity.h"
#include "Logger.h"

namespace rts {

class Renderer;
class Game;

namespace PlayerState {
  const std::string DEFAULT = "DEFAULT";
  const std::string CHATTING = "CHATTING";
}

class Player {
public:
  explicit Player(id_t playerID, const glm::vec3 &color)
    : playerID_(playerID), game_(NULL), color_(color) {
  }
  virtual ~Player() {}

  id_t getPlayerID() const {
    return playerID_;
  }

  virtual void setGame(Game *game) {
    game_ = game;
  }

  /* Called at the start of each tick, should finalize the previous frame
   * and begin preparing input for the next frame.
   * NOTE: this may be called multiple times per tick, if some players aren't
   * ready.
   *
   * @arg tick is the tick being simulated
   * @return true, if this player has submitted all input for the given frame
   */
  virtual bool update(tick_t tick) = 0;
  /* Called each time any player performs an action (including this player).
   * This function should execute quickly (i.e. not perform blocking
   * operations).
   * @arg playerID The player who performed the action.
   * @arg arction The action itself.
   */
  virtual void playerAction(id_t playerID, const PlayerAction &action) = 0;

  virtual void addChatMessage(const std::string &chat) {}
  virtual void displayChatWindow() {}

  /* Returns this player's color. */
  glm::vec3 getColor() const {
    return color_;
  }

protected:
  id_t playerID_;
  Game *game_;
  glm::vec3 color_;
};

class LocalPlayer : public Player {
public:
  LocalPlayer(id_t playerID, const glm::vec3 &color, Renderer *renderer);
  virtual ~LocalPlayer();

  virtual bool update(tick_t tick);
  virtual void setGame(Game *game);

  virtual void playerAction(id_t playerID, const PlayerAction &action);

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

  virtual void addChatMessage(const std::string &chat);
  const std::vector<std::string>& getChatMessages() const { 
    return chatMessages_; 
  }

  virtual void displayChatWindow();

  const std::string& getState() const { return state_; }

private:
  void setSelection(const std::set<id_t> &new_selection);

  Renderer *renderer_;
  // The tick the current actions will be executed on
  tick_t targetTick_;
  tick_t doneTick_;
  std::set<id_t> selection_;

  glm::vec2 cameraPanDir_;

  bool shift_;
  bool chatting_;
  std::string message_;
  std::vector<std::string> chatMessages_;
  bool leftDrag_;
  glm::vec3 leftStart_;
  std::string order_;
  std::string state_;

  LoggerPtr logger_;
};

class DummyPlayer : public Player {
public:
  DummyPlayer(id_t playerID);
  virtual bool update(tick_t tick);
  virtual void playerAction(id_t playerID, const PlayerAction &action) { }
};

// Player used for testing that occasionally drops frames
class SlowPlayer : public Player {
public:
  SlowPlayer(id_t playerID) : Player(playerID, glm::vec3(0.f)) { }

  virtual bool update(tick_t tick);
  virtual void playerAction(id_t playerID, const PlayerAction &action) { }

private:
};
}; // rts
