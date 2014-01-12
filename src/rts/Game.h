#ifndef SRC_RTS_GAME_H_
#define SRC_RTS_GAME_H_

#include <vector>
#include <mutex>
#include <glm/glm.hpp>
#include "common/Clock.h"
#include "common/Logger.h"
#include "rts/GameScript.h"

namespace rts {

class Map;
class Player;
class GameEntity;
struct ChatMessage;

class Game {
 public:
  // Should return a json array of json object messages
  // each message should have the 'type' field set at a minimum
  typedef std::function<Json::Value(void)> RenderProvider;
  typedef std::function<void(const Json::Value&)> ActionFunc;
  explicit Game(
      Map *map,
      const std::vector<Player *> &players,
      RenderProvider render_provider,
      ActionFunc action_func);
  ~Game();

  static Game* get() { return nullthrows(instance_); }

  void run();
  void addAction(id_t pid, const Json::Value &v);

  const Map * getMap() const {
    return map_;
  }
  float getElapsedTime() const {
    return elapsedTime_;
  }

  GameEntity * getEntity(const std::string &game_id);
  const GameEntity * getEntity(const std::string &game_id) const;
  const Player * getPlayer(id_t pid) const;
  const std::vector<Player *>& getPlayers() const { return players_; }

  float getRequisition(id_t pid) const;
  float getPower(id_t pid) const;
  float getVictoryPoints(id_t tid) const;

  typedef std::function<void(const ChatMessage &)> ChatListener;
  void setChatListener(ChatListener listener) {
    chatListener_ = listener;
  }

 private:
  void renderFromJSON(const Json::Value &v);
  void handleRenderMessage(const Json::Value &v);

  Map *map_;
  std::map<std::string, id_t> game_to_render_id;
  std::vector<Player *> players_;
  RenderProvider renderProvider_;
  ActionFunc actionFunc_;
  bool running_;
  // pid => float
  std::map<id_t, float> requisition_;
  std::map<id_t, float> power_;
  // tid => float
  std::map<id_t, float> victoryPoints_;
  float elapsedTime_;

  ChatListener chatListener_;

  static Game *instance_;
};

struct ChatMessage {
  ChatMessage(id_t inpid, const std::string &m, const Clock::time_point &t)
      : pid(inpid), msg(m), time(t) { }

  id_t pid;
  std::string msg;
  Clock::time_point time;
};
};  // namespace rts

#endif  // SRC_RTS_GAME_H_
