#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>
#include <json/json.h>
#include <functional>
#include <unordered_map>
#include "common/Types.h"

namespace rts {

class GameEntity;

v8::Handle<v8::Value> jsonToJS(const Json::Value &json);
Json::Value jsToJSON(const v8::Handle<v8::Value> json);
glm::ivec2 jsToIVec2(const v8::Handle<v8::Array> js);
glm::vec2 jsToVec2(const v8::Handle<v8::Array> js);
glm::vec3 jsToVec3(const v8::Handle<v8::Array> js);
v8::Handle<v8::Array> vec2ToJS(const glm::vec2 &v);


#define ENTER_GAMESCRIPT(script) \
  v8::Locker locker((script)->getIsolate()); \
  v8::HandleScope handle_scope_lol((script)->getIsolate()); \
  v8::Context::Scope context_scope((script)->getContext());

class GameScript {
public:
  static GameScript *getActiveGameScript();
  GameScript();
  ~GameScript();

  typedef std::function<v8::Handle<v8::Object>(void)> BindingFunction;
  v8::Local<v8::Value> init(
      const std::string &main_module_name,
      const std::map<std::string, BindingFunction>& extra_bindings);
  v8::Local<v8::Value> init(const std::string &main_module_name);
  v8::Local<v8::Value> getInitReturn() {
    return v8::Local<v8::Value>::New(isolate_, jsInitResult_);
  }

  v8::Isolate * getIsolate() {
    return isolate_;
  }
  v8::Local<v8::Context> getContext() {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  v8::Handle<v8::Object> getGlobal();

  // TODO(zack): replace with getBinding(const char *name) const that does
  // lazy loading of new bindings.  binding would have init method that returns
  // a value as its binding.
  // It's so embarassing that this is public, please don't call it
  v8::Local<v8::Object> getBindings() const {
    return v8::Local<v8::Object>::New(isolate_, jsBindings_);
  }

private:
  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_;

  v8::Persistent<v8::Value> jsInitResult_;
  v8::Persistent<v8::Object> jsBindings_;

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
