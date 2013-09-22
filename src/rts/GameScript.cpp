#include "rts/GameScript.h"
#include <v8.h>
#include <sstream>
#include "common/util.h"
#include "rts/Game.h"
#include "rts/GameController.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/VisibilityMap.h"

using namespace v8;

namespace rts {

void checkJSResult(
  const Handle<Value> &result,
  const TryCatch &try_catch,
  const std::string &msg) {
  if (result.IsEmpty()) {
    LOG(ERROR) << msg << " "
      << *String::AsciiValue(try_catch.Exception()) << '\n'
      << *String::AsciiValue(try_catch.StackTrace()) << '\n';
    invariant_violation("Javascript exception");
  }
}

static Handle<Value> jsGameRandom(const Arguments &args) {
  return Number::New(Game::get()->gameRandom());
}

static Handle<Value> jsSpawnRenderEntity(const Arguments &args) {
  invariant(args.Length() == 0, "object spawnRenderEntity(void)");
  HandleScope scope(args.GetIsolate());

  auto script = Game::get()->getScript();
  id_t eid = Renderer::get()->newEntityID();
  GameEntity *e = new GameEntity(eid);
  invariant(e, "couldn't allocate new entity");
  Renderer::get()->spawnEntity(e);

  Handle<Object> wrapper = script->getEntityTemplate()->NewInstance();
  wrapper->SetInternalField(0, External::New(e));

  return scope.Close(wrapper);
}

static Handle<Value> jsDestroyRenderEntity(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  invariant(args.Length() == 1, "void DestroyRenderEntity(int eid");
  HandleScope scope(args.GetIsolate());

  id_t eid = args[0]->IntegerValue();
  Game::get()->destroyEntity(eid);

  return Undefined();
}

static Handle<Value> jsLog(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  for (int i = 0; i < args.Length(); i++) {
    if (i != 0) std::cout << ' ';
    std::cout << *String::AsciiValue(args[i]);
  }
  std::cout << '\n';

  return Undefined();
}

static Handle<Value> jsAddEffect(const Arguments &args) {
  invariant(args.Length() == 2, "jsAddEffect(name, params)");
  HandleScope scope(args.GetIsolate());

  std::string name = *String::AsciiValue(args[0]);
  auto params = Handle<Object>::Cast(args[1]);

  add_jseffect(name, params);

  return Undefined();
}

static Handle<Value> jsGetNearbyEntities(const Arguments &args) {
  invariant(args.Length() == 3, "expected 3 args: pos2, radius, callback");
  HandleScope scope(args.GetIsolate());

  auto script = Game::get()->getScript();
  glm::vec3 pos = glm::vec3(script->jsToVec2(
      Handle<Array>::Cast(args[0])), 0.f);
  float radius = args[1]->NumberValue();
  Handle<Function> callback = Handle<Function>::Cast(args[2]);

  Renderer::get()->getNearbyEntities(pos, radius,
    [&](const ModelEntity *e) -> bool {
      if (!e->hasProperty(GameEntity::P_GAMEENTITY)) {
        return true;
      }
      TryCatch try_catch;
      const int argc = 1;
      Handle<Value> argv[argc] = {Integer::New(e->getID())};
      auto ret = callback->Call(args.Holder(), argc, argv);
      checkJSResult(ret, try_catch, "getNearbyEntities callback:");
      return ret->BooleanValue();
    });

  return Undefined();
}

static Handle<Value> entityContainsPoint(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());
  const glm::vec2 pt = Game::get()->getScript()->jsToVec2(
      Handle<Array>::Cast(args[0]));

  bool contains = entity->pointInEntity(pt);
  return scope.Close(Boolean::New(contains));
}

static Handle<Value> entityDistanceToPoint(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());
  const glm::vec2 target = Game::get()->getScript()->jsToVec2(
      Handle<Array>::Cast(args[0]));

  float dist = glm::distance(entity->getPosition2(), target);
  return scope.Close(Number::New(dist));
}

static Handle<Value> entityDistanceToEntity(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  Local<Object> targetobj = Local<Object>::Cast(args[0]);
  Local<External> targetwrap = Local<External>::Cast(targetobj->GetInternalField(0));
  GameEntity *target = static_cast<GameEntity *>(targetwrap->Value());

  if (!target) {
    return scope.Close(Number::New(HUGE_VAL));
  }

  float dist = entity->distanceToEntity(target);

  return scope.Close(Number::New(dist));
}

static Handle<Value> entityIsVisibleTo(const Arguments &args) {
  invariant(args.Length() == 1, "expected (id_t pid)");
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  id_t pid = args[0]->IntegerValue();

  auto vismap = Game::get()->getVisibilityMap(pid);
  bool visible = vismap->locationVisible(entity->getPosition2());

  return scope.Close(Boolean::New(visible));
}

static Handle<Value> entityRemainStationary(const Arguments &args) {
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  entity->remainStationary();

  return Undefined();
}

static Handle<Value> entityTurnTowards(const Arguments &args) {
  invariant(args.Length() == 1, "void turnTowards(vec3 point)");

  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  auto script = Game::get()->getScript();
  glm::vec2 pos = script->jsToVec2(Handle<Array>::Cast(args[0]));

  entity->turnTowards(pos);

  return Undefined();
}

static Handle<Value> entityMoveTowards(const Arguments &args) {
  invariant(args.Length() == 1, "Expected 1 arg to moveTowards");

  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  auto script = Game::get()->getScript();
  glm::vec2 pos = script->jsToVec2(Handle<Array>::Cast(args[0]));

  entity->moveTowards(pos);

  return Undefined();
}

static Handle<Value> entityWarpPosition(const Arguments &args) {
  if (args.Length() < 1) return Undefined();

  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  auto script = Game::get()->getScript();
  glm::vec2 pos = script->jsToVec2(Handle<Array>::Cast(args[0]));

  entity->warpPosition(pos);

  return Undefined();
}

static Handle<Value> entityOnEvent(const Arguments &args) {
  if (args.Length() != 2) {
    invariant_violation("entity.onEvent(name, params)");
  }

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  HandleScope scope(args.GetIsolate());
  std::string name(*String::AsciiValue(args[0]));

  // TODO(zack); kill this entire function, use the global version
  auto effect = makeEntityEffect(
        entity,
        name,
        Handle<Object>::Cast(args[1]));
  if (effect) {
    entity->addExtraEffect(effect);
  }

  return Undefined();
}

static Handle<Value> entityHasProperty(const Arguments &args) {
  if (args.Length() < 1) return Undefined();

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *game_entity = static_cast<GameEntity *>(wrap->Value());
  return scope.Close(Boolean::New(game_entity->hasProperty(args[0]->Uint32Value())));
}

static Handle<Value> entityGetID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getID());
  return scope.Close(ret);
}

static Handle<Value> entityGetTeamID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getTeamID());
  return scope.Close(ret);
}

static Handle<Value> entityGetPosition2(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  auto pos2 = e->getPosition2();
  Handle<Array> ret = Array::New(2);
  ret->Set(0, Number::New(pos2.x));
  ret->Set(1, Number::New(pos2.y));
  return scope.Close(ret);
}

static Handle<Value> entityGetDirection(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  auto pos2 = e->getDirection();
  Handle<Array> ret = Array::New(2);
  ret->Set(0, Number::New(pos2.x));
  ret->Set(1, Number::New(pos2.y));
  return scope.Close(ret);
}

static Handle<Value> entityGetAngle(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Value> ret = Number::New(e->getAngle());
  return scope.Close(ret);
}

static Handle<Value> entitySetModel(const Arguments &args) {
  invariant(args.Length() == 1, "setModel(string model)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setModelName(*String::AsciiValue(args[0]));
  return Undefined();
}

static Handle<Value> entitySetMaxSpeed(const Arguments &args) {
  invariant(args.Length() == 1, "setMaxSpeed takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setMaxSpeed(args[0]->NumberValue());
  return Undefined();
}

static Handle<Value> entitySetSight(const Arguments &args) {
  invariant(args.Length() == 1, "setSetSight takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());

  e->setSight(args[0]->NumberValue());
  return Undefined();
}

static Handle<Value> entitySetSize(const Arguments &args) {
  invariant(args.Length() == 1, "setSetSize takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  glm::vec3 size = Game::get()->getScript()->jsToVec3(
      Handle<Array>::Cast(args[0]));
  e->setSize(glm::vec2(size));
  e->setHeight(size.z);

  return Undefined();
}

static Handle<Value> entitySetProperties(const Arguments &args) {
  invariant(args.Length() == 1, "void entitySetProperties(array properties)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  auto js_properties = Handle<Array>::Cast(args[0]);
  for (int i = 0; i < js_properties->Length(); i++) {
    e->addProperty(js_properties->Get(Integer::New(i))->IntegerValue());
  }

  return Undefined();
}
static Handle<Value> entitySetPlayerID(const Arguments &args) {
  if (args.Length() < 1) return Undefined();

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());
  auto player = Game::get()->getPlayer(args[0]->IntegerValue());
  if (player) {
    e->setPlayerID(player->getPlayerID());
  }
  return Undefined();
}

GameScript::GameScript()
  : isolate_(nullptr) {
}

GameScript::~GameScript() {
  {
    Locker locker(isolate_);
    HandleScope handle_scope(isolate_);
    Context::Scope context_scope(isolate_, context_);

    entityTemplate_.Dispose();
    context_.Dispose(isolate_);
  }
  isolate_->Exit();
  isolate_->Dispose();
}

void GameScript::init() {
  const std::string v8_flags = "--use_strict";
  v8::V8::SetFlagsFromString(v8_flags.c_str(), v8_flags.length());
  isolate_ = Isolate::New();
  Locker locker(isolate_);
  isolate_->Enter();

  HandleScope handle_scope(isolate_);

  Handle<ObjectTemplate> global = ObjectTemplate::New();

  global->Set(
      String::New("Log"),
      FunctionTemplate::New(jsLog));
  global->Set(
      String::New("GameRandom"),
      FunctionTemplate::New(jsGameRandom));
  global->Set(
      String::New("SpawnRenderEntity"),
      FunctionTemplate::New(jsSpawnRenderEntity));
  global->Set(
      String::New("DestroyRenderEntity"),
      FunctionTemplate::New(jsDestroyRenderEntity));
  global->Set(
      String::New("GetNearbyEntities"),
      FunctionTemplate::New(jsGetNearbyEntities));
  global->Set(
      String::New("AddEffect"),
      FunctionTemplate::New(jsAddEffect));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->Set(
      String::New("hasProperty"),
      FunctionTemplate::New(entityHasProperty));
  entityTemplate_->Set(
      String::New("getID"),
      FunctionTemplate::New(entityGetID));
  entityTemplate_->Set(
      String::New("getTeamID"),
      FunctionTemplate::New(entityGetTeamID));
  entityTemplate_->Set(
      String::New("getPosition2"),
      FunctionTemplate::New(entityGetPosition2));
  entityTemplate_->Set(
      String::New("getDirection"),
      FunctionTemplate::New(entityGetDirection));
  entityTemplate_->Set(
      String::New("getAngle"),
      FunctionTemplate::New(entityGetAngle));

  entityTemplate_->Set(
      String::New("setModel"),
      FunctionTemplate::New(entitySetModel));
  entityTemplate_->Set(
      String::New("setMaxSpeed"),
      FunctionTemplate::New(entitySetMaxSpeed));
  entityTemplate_->Set(
      String::New("setSize"),
      FunctionTemplate::New(entitySetSize));
  entityTemplate_->Set(
      String::New("setSight"),
      FunctionTemplate::New(entitySetSight));

  entityTemplate_->Set(
      String::New("setProperties"),
      FunctionTemplate::New(entitySetProperties));
  entityTemplate_->Set(
      String::New("setPlayerID"),
      FunctionTemplate::New(entitySetPlayerID));
  entityTemplate_->Set(
      String::New("remainStationary"),
      FunctionTemplate::New(entityRemainStationary));
  entityTemplate_->Set(
      String::New("turnTowards"),
      FunctionTemplate::New(entityTurnTowards));
  entityTemplate_->Set(
      String::New("moveTowards"),
      FunctionTemplate::New(entityMoveTowards));
  entityTemplate_->Set(
      String::New("warpPosition"),
      FunctionTemplate::New(entityWarpPosition));

  entityTemplate_->Set(
      String::New("containsPoint"),
      FunctionTemplate::New(entityContainsPoint));
  entityTemplate_->Set(
      String::New("distanceToEntity"),
      FunctionTemplate::New(entityDistanceToEntity));
  entityTemplate_->Set(
      String::New("distanceToPoint"),
      FunctionTemplate::New(entityDistanceToPoint));
  entityTemplate_->Set(
      String::New("isVisibleTo"),
      FunctionTemplate::New(entityIsVisibleTo));

  entityTemplate_->Set(
      String::New("onEvent"),
      FunctionTemplate::New(entityOnEvent));

  loadScripts();
}

void GameScript::loadScripts() {
  auto scripts = ResourceManager::get()->getOrderedScriptNames();
  for (const auto &pair : scripts) {
    std::string filename = pair.first;
    std::string contents = pair.second;

    LOG(INFO) << "Loading script: " << filename << '\n';

    TryCatch try_catch;
    Handle<Script> script = Script::Compile(String::New(contents.c_str()));
    if (script.IsEmpty()) {
      String::AsciiValue e_str(try_catch.Exception());
      LOG(ERROR) << "Unable to compile script '" << filename << "': " << 
          *e_str << '\n';
      invariant_violation("Error loading javascript");
    }

    Handle<Value> result = script->Run();
    if (result.IsEmpty()) {
      String::AsciiValue e_str(try_catch.Exception());
      LOG(ERROR) << "Unable to run script '" << filename << "': " << 
          *e_str << '\n';
      invariant_violation("Error loading javascript");
    }
  }
}

Handle<Object> GameScript::getGlobal() {
  return context_->Global();
}

Handle<Value> GameScript::jsonToJS(const Json::Value &json) const {
  if (json.isArray()) {
    Handle<Array> jsarr = Array::New();
    for (int i = 0; i < json.size(); i++) {
      jsarr->Set(i, jsonToJS(json[i]));
    }
    return jsarr;
  } else if (json.isObject()) {
    Handle<Object> jsobj = Object::New();
    for (const auto &name : json.getMemberNames()) {
      auto jsname = String::New(name.c_str());
      jsobj->Set(jsname, jsonToJS(json[name]));
    }
    return jsobj;
  } else if (json.isDouble()) {
    return Number::New(json.asDouble());
  } else if (json.isString()) {
    return String::New(json.asCString());
  } else if (json.isIntegral()) {
    return Integer::New(json.asInt64());
  } else if (json.isBool()) {
    return Boolean::New(json.asBool());
  } else {
    invariant_violation("Unknown type to convert");
  }
}

Json::Value GameScript::jsToJSON(const Handle<Value> js) const {
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

glm::vec2 GameScript::jsToVec2(const Handle<Array> arr) const {
  glm::vec2 ret;

  if (arr->Length() != 2) {
    LOG(WARNING) << "Trying to convert array of size "
      << arr->Length() << " to vec2\n";
    return ret;
  }

  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  return ret;
}
glm::vec3 GameScript::jsToVec3(const Handle<Array> arr) const {
  glm::vec3 ret;

  if (arr->Length() != 3) {
    LOG(WARNING) << "Trying to convert array of size "
      << arr->Length() << " to vec3\n";
    return ret;
  }

  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  ret.z = arr->Get(2)->NumberValue();
  return ret;
}

Handle<Array> GameScript::vec2ToJS(const glm::vec2 &v) const {
  Handle<Array> ret = Array::New();
  ret->Set(0, Number::New(v[0]));
  ret->Set(1, Number::New(v[1]));

  return ret;
}
};  // rts
