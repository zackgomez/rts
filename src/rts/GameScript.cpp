#include "rts/GameScript.h"
#include <v8.h>
#include <sstream>
#include "common/Collision.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/NativeUIBinding.h"
#include "rts/ResourceManager.h"

using namespace v8;

namespace rts {

static const std::string BOOTSTRAP_FILE = "jscore/bootstrap.js";

GameScript *GameScript::getActiveGameScript() {
  auto isolate = Isolate::GetCurrent();
  invariant(isolate, "must have active isolate");
  return (GameScript *)isolate->GetData();
}

void jsFail(const v8::TryCatch &try_catch, const std::string &msg) {
  LOG(ERROR) << msg << " "
    << *String::AsciiValue(try_catch.Exception()) << '\n'
    << *String::AsciiValue(try_catch.StackTrace()) << '\n';
  invariant_violation("Javascript exception");
}

static Handle<Value> runtimeEvalImpl(
    Handle<String> source,
    Handle<Object> kwargs);

static void runtimeEval(const FunctionCallbackInfo<Value> &args) {
  if (args.Length() < 2) {
    invariant_violation("value runtimeEval(source, kwargs)");
  }
  HandleScope scope(args.GetIsolate());

  auto js_source = args[0]->ToString();
  auto js_kwargs = args[1]->ToObject();
  args.GetReturnValue().Set(
      scope.Close(runtimeEvalImpl(js_source, js_kwargs)));
}

static void jsPointInOBB2(const FunctionCallbackInfo<Value> &args);
static Handle<Object> getCollisionBinding() {
  HandleScope scope(Isolate::GetCurrent());
  auto binding = Object::New();
  binding->Set(
      String::New("pointInOBB2"),
      FunctionTemplate::New(jsPointInOBB2)->GetFunction());

  return scope.Close(binding);
}

static void jsResolveCollisions(const FunctionCallbackInfo<Value> &args);
static Handle<Object> getPathingBinding() {
  HandleScope scope(Isolate::GetCurrent());
  auto binding = Object::New();
  binding->Set(
      String::New("resolveCollisions"),
      FunctionTemplate::New(jsResolveCollisions)->GetFunction());

  return scope.Close(binding);
}

static void jsRuntimeBinding(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "value runtime.binding(string name)");
  HandleScope scope(args.GetIsolate());

  auto name = Handle<String>::Cast(args[0]);
  auto binding_map = GameScript::getActiveGameScript()->getBindings();
  invariant(
    binding_map->Has(args[0]),
    std::string("unknown binding ") + *String::AsciiValue(name));
  args.GetReturnValue().Set(scope.Close(binding_map->Get(name)));
}

static void jsLog(const FunctionCallbackInfo<Value> &args) {
  HandleScope scope(args.GetIsolate());
  for (int i = 0; i < args.Length(); i++) {
    if (i != 0) std::cout << ' ';
    std::cout << *String::AsciiValue(args[i]);
  }
  std::cout << '\n';

  args.GetReturnValue().SetUndefined();
}

static void jsPointInOBB2(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 4, "bool pointInOBB2(p, center, size, angle)");
  HandleScope scope(args.GetIsolate());

  auto pt = jsToVec2(Handle<Array>::Cast(args[0]));
  auto center = jsToVec2(Handle<Array>::Cast(args[1]));
  auto size = jsToVec2(Handle<Array>::Cast(args[2]));
  auto angle = args[3]->NumberValue();

  bool contains = pointInBox(pt, center, size, angle);
  args.GetReturnValue().Set(scope.Close(Boolean::New(contains)));
}

static void jsResolveCollisions(const FunctionCallbackInfo<Value> &args) {
  invariant(
      args.Length() == 3,
      "resolveCollisions(map<key, objecy> bodies, float dt, func callback): void");
  HandleScope scope(args.GetIsolate());
  auto global = Context::GetCalling()->Global();

  auto pos_str = String::New("pos");
  auto size_str = String::New("size");
  auto vel_str = String::New("pos");
  auto angle_str = String::New("angle");

  auto bodies = Handle<Object>::Cast(args[0]);
  float dt = args[1]->NumberValue();
  auto callback = Handle<Function>::Cast(args[2]);

  auto keys = bodies->GetOwnPropertyNames();
  for (auto i = 0; i < keys->Length(); i++) {
    auto key1 = keys->Get(i);
    auto body1 = Handle<Object>::Cast(bodies->Get(key1));
    auto pos1 = jsToVec2(Handle<Array>::Cast(body1->Get(pos_str)));
    auto size1 = jsToVec2(Handle<Array>::Cast(body1->Get(size_str)));
    auto angle1 = body1->Get(angle_str)->NumberValue();
    auto vel1 = jsToVec2(Handle<Array>::Cast(body1->Get(vel_str)));
    Rect rect1(pos1, size1, angle1);

    for (auto j = i+1; j < keys->Length(); j++) {
      auto key2 = keys->Get(j);
      auto body2 = Handle<Object>::Cast(bodies->Get(key2));
      auto pos2 = jsToVec2(Handle<Array>::Cast(body2->Get(pos_str)));
      auto size2 = jsToVec2(Handle<Array>::Cast(body2->Get(size_str)));
      auto angle2 = body2->Get(angle_str)->NumberValue();
      auto vel2 = jsToVec2(Handle<Array>::Cast(body2->Get(vel_str)));
      Rect rect2(pos2, size2, angle2);

      float time;
      if ((time = boxBoxCollision(
            rect1,
            vel1,
            rect2,
            vel2,
            dt)) != NO_INTERSECTION) {
        const int argc = 3;
        Handle<Value> argv[] = { key1, key2, Number::New(time) };

        callback->Call(global, argc, argv);
      }
    }
  }
  args.GetReturnValue().SetUndefined();
}

GameScript::GameScript()
  : isolate_(nullptr) {
}

GameScript::~GameScript() {
  {
    Locker locker(isolate_);
    HandleScope handle_scope(isolate_);
    Context::Scope context_scope(isolate_, context_);

    jsBindings_.Dispose();
    context_.Reset();
  }
  isolate_->Exit();
  isolate_->Dispose();
}

class MyAllocator : public v8::ArrayBuffer::Allocator {
public:
  virtual ~MyAllocator() { }
  virtual void* Allocate(size_t length) override {
    return calloc(1, length);
  }
  virtual void* AllocateUninitialized(size_t length) override {
    return malloc(length);
  }
  virtual void Free(void* data, size_t length) override {
    free(data);
  }
};

void configure_v8() {
  static bool configured = false;
  if (configured) return;
  
  configured = true;
  const int v8_argc = 4;
  int argc = v8_argc;
  char *argv[] = {
    NULL,
    strdup("--use_strict"),
    strdup("--harmony_array_buffer"),
    strdup("--harmony_typed_arrays"),
  };
  v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
  for (int i = 0; i < v8_argc; i++) {
//    free(argv[i]);
  }

  V8::SetArrayBufferAllocator(new MyAllocator());
}

v8::Local<v8::Value> GameScript::init(const std::string &main_module_name) {
  configure_v8();

  isolate_ = Isolate::New();
  Locker locker(isolate_);
  isolate_->Enter();
  isolate_->SetData(this);

  HandleScope handle_scope(isolate_);
  TryCatch try_catch;

  // TODO(zack): move these into a binding
  Handle<ObjectTemplate> global = ObjectTemplate::New();
  global->Set(
      String::New("Log"),
      FunctionTemplate::New(jsLog));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  // init bindings
  auto bindings = Object::New();
  bindings->Set(
      String::New("collision"),
      getCollisionBinding());
  bindings->Set(
      String::New("pathing"),
      getPathingBinding());
  bindings->Set(
      String::New("nativeui"),
      getNativeUIBinding());
  jsBindings_.Reset(isolate_, bindings);
  // set up runtime object
  auto source_map = getSourceMap();
  auto runtime_object = Object::New();
  runtime_object->Set(
      String::New("binding"),
      FunctionTemplate::New(jsRuntimeBinding)->GetFunction());
  runtime_object->Set(
      String::New("source_map"),
      source_map);
  runtime_object->Set(
      String::New("eval"),
      FunctionTemplate::New(runtimeEval)->GetFunction());
  getContext()->Global()->Set(
      String::New("runtime"),
      runtime_object);


  std::ifstream bootstrap_file(BOOTSTRAP_FILE);
  std::string bootstrap_source;
  std::getline(bootstrap_file, bootstrap_source, (char)EOF);

  auto kwargs = Object::New();
  kwargs->Set(
      String::New("filename"),
      String::New(BOOTSTRAP_FILE.c_str()));

  auto bootstrap_func_val = runtimeEvalImpl(
      String::New(bootstrap_source.c_str()), 
      kwargs);
  checkJSResult(bootstrap_func_val, try_catch, "load bootstrapper");
  invariant(
      bootstrap_func_val->IsFunction(),
      "init file must evaluate to a function");
  const int argc = 2;
  Handle<Value> argv[] = {
    runtime_object,
    String::New(main_module_name.c_str()),
  };
  auto ret = Handle<Function>::Cast(bootstrap_func_val)
    ->Call(getGlobal(), argc, argv);
  checkJSResult(ret, try_catch, "bootstrap");
  jsInitResult_.Reset(isolate_, ret);
  return Local<Value>::New(isolate_, jsInitResult_);
}


Handle<Value> runtimeEvalImpl(Handle<String> source, Handle<Object> kwargs) {
  TryCatch try_catch;

  auto jsfilename = kwargs->Get(String::New("filename"));
  auto filename = *String::AsciiValue(jsfilename);

  Handle<Script> script = Script::Compile(source, jsfilename);
  checkJSResult(script, try_catch, std::string("compile: ") + filename);

  Handle<Value> result = script->Run();
  checkJSResult(result, try_catch, "execute:");

  return result;
}

Handle<Object> GameScript::getSourceMap() const {
  Handle<Object> js_source_map = Object::New();

  auto scripts = ResourceManager::get()->readScripts();
  for (auto pair : scripts) {
    js_source_map->Set(
        String::New(pair.first.c_str()),
        String::New(pair.second.c_str()));
  }

  return js_source_map;
}

Handle<Object> GameScript::getGlobal() {
  return getContext()->Global();
}

Handle<Value> jsonToJS(const Json::Value &json) {
  HandleScope handle_scope(Isolate::GetCurrent());
  Handle<Value> ret;
  if (json.isArray()) {
    Handle<Array> jsarr = Array::New();
    for (int i = 0; i < json.size(); i++) {
      jsarr->Set(i, jsonToJS(json[i]));
    }
    ret = jsarr;
  } else if (json.isObject()) {
    Handle<Object> jsobj = Object::New();
    for (const auto &name : json.getMemberNames()) {
      auto jsname = String::New(name.c_str());
      jsobj->Set(jsname, jsonToJS(json[name]));
    }
    ret = jsobj;
  } else if (json.isDouble()) {
    ret = Number::New(json.asDouble());
  } else if (json.isString()) {
    ret = String::New(json.asCString());
  } else if (json.isIntegral()) {
    ret = Integer::New(json.asInt64());
  } else if (json.isBool()) {
    ret = Boolean::New(json.asBool());
  } else {
    invariant_violation("Unknown type to convert");
  }

  return handle_scope.Close(ret);
}

Json::Value jsToJSON(const Handle<Value> js) {
  if (js->IsArray()) {
    Json::Value ret;
    auto jsarr = Handle<Array>::Cast(js);
    for (int i = 0; i < jsarr->Length(); i++) {
      ret[i] = jsToJSON(jsarr->Get(i));
    }
    return ret;
  } else if (js->IsObject()) {
    Json::Value ret;
    auto jsobj = js->ToObject();
    Handle<Array> names = jsobj->GetPropertyNames();
    for (int i = 0; i < names->Length(); i++) {
      ret[*String::AsciiValue(names->Get(i))] =
        jsToJSON(jsobj->Get(names->Get(i)));
    }
    return ret;
  } else if (js->IsNumber()) {
    return js->NumberValue();
  } else if (js->IsString()) {
    return *String::AsciiValue(js);
  } else if (js->IsInt32() || js->IsUint32()) {
    return Json::Value(js->Int32Value());
  } else if (js->IsBoolean()) {
    return js->BooleanValue();
  } else if (js->IsNull()) {
    return Json::Value();
  } else {
    invariant_violation("Unknown js type");
  }
}

glm::ivec2 jsToIVec2(const Handle<Array> arr) {
  invariant(
      arr->Length() == 2,
      "trying to convert an incorrectly sized array to ivec2");

  glm::ivec2 ret;
  ret.x = arr->Get(0)->ToInt32()->Int32Value();
  ret.y = arr->Get(1)->ToInt32()->Int32Value();
  return ret;
}
glm::vec2 jsToVec2(const Handle<Array> arr) {
  invariant(
      arr->Length() == 2,
      "trying to convert an incorrectly sized array to vec2");

  glm::vec2 ret;
  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  return ret;
}
glm::vec3 jsToVec3(const Handle<Array> arr) {
  invariant(
      arr->Length() == 3,
      "trying to convert an incorrectly sized array to vec2");

  glm::vec3 ret;
  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  ret.z = arr->Get(2)->NumberValue();
  return ret;
}

Handle<Array> vec2ToJS(const glm::vec2 &v) {
  HandleScope scope(Isolate::GetCurrent());
  Handle<Array> ret = Array::New();
  ret->Set(0, Number::New(v[0]));
  ret->Set(1, Number::New(v[1]));

  return scope.Close(ret);
}
};  // rts
