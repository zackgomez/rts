#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>
#include <unordered_map>

namespace rts {

class GameEntity;

class GameScript {
public:
  ~GameScript();
  void init();

  void wrapEntity(GameEntity *e);
  void destroyEntity(GameEntity *e);

private:
  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_ = nullptr;

  v8::Persistent<v8::ObjectTemplate> entityTemplate_;

  std::unordered_map<GameEntity *, v8::Persistent<v8::Object>> scriptObjects_;
};

};

#endif  // SRC_RTS_GAMESCRIPT_H_
