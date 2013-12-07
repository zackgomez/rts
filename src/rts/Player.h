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
class GameEntity;
class VisibilityMap;
class Player {
 public:
  explicit Player(id_t playerID, id_t teamID, const std::string &name,
      const glm::vec3 &color)
      : playerID_(playerID), teamID_(teamID), name_(name), color_(color),
        game_(nullptr) {
    assertPid(playerID_);
    assertTid(teamID_);
  }

  virtual ~Player() { }

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

  virtual bool isLocal() const {
    return false;
  }

 protected:
  id_t playerID_;
  id_t teamID_;
  const std::string name_;
  glm::vec3 color_;

  Game *game_;
};

// TODO(zack): find a home for this function
bool isControlGroupHotkey(int hotkey);

class LocalPlayer : public Player {
 public:
  LocalPlayer(id_t playerID, id_t teamID, const std::string &name,
              const glm::vec3 &color);
  virtual ~LocalPlayer();

  virtual std::vector<PlayerAction> getActions();

  virtual void playerAction(id_t playerID, const PlayerAction &action);

  virtual void startTick(tick_t tick);
  virtual bool isReady() const;

  virtual bool isLocal() const {
    return true;
  }

  void setSelection(const std::set<std::string> &selection) {
    selection_ = selection;
  }
  bool isSelected(const std::string &game_id) const {
    return selection_.find(game_id) != selection_.end();
  }
  const std::set<std::string>& getSelection() const {
    return selection_;
  }

  virtual void addSavedSelection(char hotkey, const std::set<std::string> &sel) {
    savedSelections_[hotkey] = sel;
  }
  const std::map<char, std::set<std::string>> getSavedSelections() const {
    return savedSelections_;
  }
  std::set<std::string> getSavedSelection(char hotkey) const {
    auto it = savedSelections_.find(hotkey);
    if (it == savedSelections_.end()) {
      return std::set<std::string>();
    }
    return it->second;
  }


 private:
  void addAction(const PlayerAction &a);

  std::queue<PlayerAction> actions_;
  std::mutex actionMutex_;
  std::set<std::string> selection_;
  std::map<char, std::set<std::string>> savedSelections_;
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
