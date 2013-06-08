#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>
#include <json/json.h>
#include <unordered_map>
#include "common/Types.h"

namespace rts {

class GameEntity;

void checkJSResult(
  const v8::Handle<v8::Value> &result,
  const v8::Handle<v8::Value> &exception,
  const std::string &msg);

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

  void wrapEntity(GameEntity *e, const Json::Value &params);
  void destroyEntity(id_t eid);
  v8::Handle<v8::Object> getEntity(id_t eid);
  v8::Handle<v8::Object> getGlobal();

  v8::Handle<v8::Value> jsonToJS(const Json::Value &json) const;
  Json::Value jsToJSON(const v8::Handle<v8::Value> json) const;
  glm::vec2 jsToVec2(const v8::Handle<v8::Array> js) const;
  glm::vec3 jsToVec3(const v8::Handle<v8::Array> js) const;

private:
  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_;

  v8::Persistent<v8::ObjectTemplate> entityTemplate_;

  std::unordered_map<id_t, v8::Persistent<v8::Object>> scriptObjects_;

  void loadScripts();
};

};

#endif  // SRC_RTS_GAMESCRIPT_H_
