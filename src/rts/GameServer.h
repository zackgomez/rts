#ifndef SRC_RTS_GAMESERVER_H_
#define SRC_RTS_GAMESERVER_H_
#include "rts/GameScript.h"
#include "rts/PlayerAction.h"

namespace rts {

class GameServer {
 public:
  GameServer();
  ~GameServer();

  bool isRunning() {
    return running_;
  }

  // Can possibly block, but should never block long
  void addAction(const PlayerAction &act);
  void start(const Json::Value &game_def);
  Json::Value update(float dt);

 private:
  v8::Handle<v8::Object> getGameObject();
  void updateJS(v8::Handle<v8::Array> player_inputs, float dt);

  GameScript *script_;
  bool running_;

  std::mutex actionMutex_;
  std::vector<PlayerAction> actions_;
};

};  // rts
#endif  // SRC_RTS_GAMESERVER_H_