#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>
#include <unordered_map>
#include "common/Types.h"

namespace rts {

class GameEntity;

class GameScript {
public:
  GameScript();
  ~GameScript();
  void init();

  v8::Isolate * getIsolate() {
    return isolate_;
  }
  v8::Persistent<v8::Context> getContext() {
    return context_;
  }

  void wrapEntity(GameEntity *e);
  void destroyEntity(GameEntity *e);
  v8::Handle<v8::Object> getEntity(id_t eid);
  v8::Handle<v8::Object> getGlobal();

private:
  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_;

  v8::Persistent<v8::ObjectTemplate> entityTemplate_;

  std::unordered_map<id_t, v8::Persistent<v8::Object>> scriptObjects_;

  void loadScripts();
};

};

#endif  // SRC_RTS_GAMESCRIPT_H_
