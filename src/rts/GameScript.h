#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>
#include <json/json.h>
#include <unordered_map>
#include "common/Types.h"

namespace rts {

class GameEntity;

class GameScript {
public:
  GameScript();
  ~GameScript();
  void init(const std::string &main_module_name);

  v8::Isolate * getIsolate() {
    return isolate_;
  }
  v8::Persistent<v8::Context> getContext() {
    return context_;
  }

  v8::Handle<v8::Object> getGlobal();

  v8::Handle<v8::Value> jsonToJS(const Json::Value &json) const;
  Json::Value jsToJSON(const v8::Handle<v8::Value> json) const;
  glm::vec2 jsToVec2(const v8::Handle<v8::Array> js) const;
  glm::vec3 jsToVec3(const v8::Handle<v8::Array> js) const;
  v8::Handle<v8::Array> vec2ToJS(const glm::vec2 &v) const;

  // TODO(zack): replace with getBinding(const char *name) const that does
  // lazy loading of new bindings.  binding would have init method that returns
  // a value as its binding.
  // It's so embarassing that this is public, please don't call it
  v8::Handle<v8::Object> getBindings() const {
    return jsBindings_;
  }
  // TODO(zack): should be private, need to replace entity with binding
  v8::Handle<v8::ObjectTemplate> getEntityTemplate() const {
    return entityTemplate_;
  }

private:

  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_;

  v8::Persistent<v8::Object> jsBindings_;
  v8::Persistent<v8::ObjectTemplate> entityTemplate_;

  v8::Handle<v8::Object> getSourceMap() const;
};

void jsFail(const v8::TryCatch &try_catch, const std::string &msg);

template<typename T> void checkJSResult(
  const v8::Handle<T> &result,
  const v8::TryCatch &try_catch,
  const std::string &msg) {
  if (result.IsEmpty()) {
    jsFail(try_catch, msg);
  }
}
};

#endif  // SRC_RTS_GAMESCRIPT_H_
