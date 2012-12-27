#ifndef SRC_RTS_PLAYER_H_
#define SRC_RTS_PLAYER_H_

#include <set>
#include <string>
#include <queue>
#include <mutex>
#include <glm/glm.hpp>
#include <json/json.h>
#include "common/Logger.h"
#include "common/Types.h"
#include "rts/PlayerAction.h"

namespace rts {

class Game;
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

  virtual void playerAction(id_t playerID, const PlayerAction &action);

  virtual void startTick(tick_t tick);
  virtual bool isReady() const;

  void setSelection(const std::set<id_t> &selection) {
    selection_ = selection;
  }
  void setSelection(std::set<id_t> &&selection) {
    selection_ = std::move(selection);
  }
  bool isSelected(id_t eid) const {
    return selection_.find(eid) != selection_.end();
  }
  const std::set<id_t>& getSelection() const {
    return selection_;
  }

 private:
  void addAction(const PlayerAction &a);

  std::queue<PlayerAction> actions_;
  std::mutex actionMutex_;
  std::set<id_t> selection_;
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
