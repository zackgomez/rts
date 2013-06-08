#include "rts/GameScript.h"
#include <v8.h>
#include "common/util.h"
#include "rts/Actor.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"

using namespace v8;

namespace rts {

void checkJSResult(
  const Handle<Value> &result,
  const Handle<Value> &exception,
  const std::string &msg) {
  if (result.IsEmpty()) {
    LOG(ERROR) << msg << " "
      << *String::AsciiValue(exception) << '\n';
    invariant_violation("Javascript exception");
  }
}

static Handle<Value> jsGetEntity(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  auto script = Game::get()->getScript();

  GameEntity *e = Game::get()->getEntity(args[0]->IntegerValue());
  if (!e) {
    return Null();
  }
  return scope.Close(script->getEntity(e->getID()));
}

static Handle<Value> jsSpawnEntity(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  auto script = Game::get()->getScript();
  std::string name(*String::AsciiValue(args[0]));
  Json::Value params = script->jsToJSON(Handle<Object>::Cast(args[1]));

  const GameEntity *e = Game::get()->spawnEntity(name, params);
  if (!e) {
    return Null();
  }
  return scope.Close(Integer::New(e->getID()));
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

static Handle<Value> jsGetRequisition(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t pid = args[0]->IntegerValue();
  return scope.Close(Number::New(
        Game::get()->getResources(pid).requisition));
}

static Handle<Value> jsAddRequisition(const Arguments &args) {
  if (args.Length() < 3) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t pid = args[0]->IntegerValue();
  float amount = args[1]->NumberValue();
  id_t from_eid = args[2]->IntegerValue();
  Game::get()->addResources(pid, ResourceType::REQUISITION, amount, from_eid);

  return Undefined();
}

static Handle<Value> jsAddVPs(const Arguments &args) {
  if (args.Length() < 3) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t pid = args[0]->IntegerValue();
  float amount = args[1]->NumberValue();
  id_t from_eid = args[2]->IntegerValue();
  Game::get()->addVPs(pid, amount, from_eid);

  return Undefined();
}

static Handle<Value> entityGetNearbyEntities(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  float radius = args[0]->NumberValue();
  Handle<Function> callback = Handle<Function>::Cast(args[1]);

  auto pos = entity->getPosition();

  const int argc = 1;
  Renderer::get()->getNearbyEntities(pos, radius,
      [&](const ModelEntity *e) -> bool {
        TryCatch try_catch;
        auto jsEntity = Game::get()->getScript()->getEntity(e->getID());
        Handle<Value> argv[1] = {jsEntity};
        auto ret = callback->Call(args.Holder(), argc, argv);
        checkJSResult(ret, try_catch.Exception(), "getNearbyEntities callback:");
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

static Handle<Value> entityRemainStationary(const Arguments &args) {
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  entity->remainStationary();

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


static Handle<Value> entityDestroy(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  Game::get()->destroyEntity(actor->getID());
  return Undefined();
}

static Handle<Value> entityHasProperty(const Arguments &args) {
  if (args.Length() < 1) return Undefined();

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  return scope.Close(Boolean::New(actor->hasProperty(args[0]->Uint32Value())));
}

static Handle<Value> entityGetName(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  return scope.Close(String::New(actor->getName().c_str()));
}

static Handle<Value> entityGetID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getID());
  return scope.Close(ret);
}

static Handle<Value> entityGetPlayerID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getPlayerID());
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
  Actor *e = static_cast<Actor *>(wrap->Value());

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

static Handle<Value> entitySetProperty(const Arguments &args) {
  if (args.Length() < 2) return Undefined();

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  e->setProperty(args[0]->IntegerValue(), args[1]->BooleanValue());
  return Undefined();
}
static Handle<Value> entitySetPlayerID(const Arguments &args) {
  if (args.Length() < 1) return Undefined();

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *e = static_cast<Actor *>(wrap->Value());
  auto player = Game::get()->getPlayer(args[0]->IntegerValue());
  if (player) {
    e->setPlayerID(player->getPlayerID());
  }
  return Undefined();
}

static Handle<Value> entityOnTookDamage(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *e = static_cast<Actor *>(wrap->Value());
  e->setTookDamage();
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
    for (auto pair : scriptObjects_) {
      pair.second.Dispose();
    }
    context_.Dispose(isolate_);
  }
  isolate_->Exit();
  isolate_->Dispose();
}

void GameScript::init() {
  isolate_ = Isolate::New();
  Locker locker(isolate_);
  isolate_->Enter();

  HandleScope handle_scope(isolate_);

  Handle<ObjectTemplate> global = ObjectTemplate::New();

  global->Set(
      String::New("Log"),
      FunctionTemplate::New(jsLog));
  global->Set(
      String::New("GetRequisition"),
      FunctionTemplate::New(jsGetRequisition));
  global->Set(
      String::New("AddRequisition"),
      FunctionTemplate::New(jsAddRequisition));
  global->Set(
      String::New("AddVPs"),
      FunctionTemplate::New(jsAddVPs));
  global->Set(
      String::New("GetEntity"),
      FunctionTemplate::New(jsGetEntity));
  global->Set(
      String::New("SpawnEntity"),
      FunctionTemplate::New(jsSpawnEntity));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->Set(
      String::New("getName"),
      FunctionTemplate::New(entityGetName));
  entityTemplate_->Set(
      String::New("hasProperty"),
      FunctionTemplate::New(entityHasProperty));
  entityTemplate_->Set(
      String::New("getID"),
      FunctionTemplate::New(entityGetID));
  entityTemplate_->Set(
      String::New("getPlayerID"),
      FunctionTemplate::New(entityGetPlayerID));
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
      String::New("setMaxSpeed"),
      FunctionTemplate::New(entitySetMaxSpeed));
  entityTemplate_->Set(
      String::New("setSize"),
      FunctionTemplate::New(entitySetSize));
  entityTemplate_->Set(
      String::New("setSight"),
      FunctionTemplate::New(entitySetSight));

  entityTemplate_->Set(
      String::New("setProperty"),
      FunctionTemplate::New(entitySetProperty));
  entityTemplate_->Set(
      String::New("setPlayerID"),
      FunctionTemplate::New(entitySetPlayerID));
  entityTemplate_->Set(
      String::New("remainStationary"),
      FunctionTemplate::New(entityRemainStationary));
  entityTemplate_->Set(
      String::New("moveTowards"),
      FunctionTemplate::New(entityMoveTowards));
  entityTemplate_->Set(
      String::New("warpPosition"),
      FunctionTemplate::New(entityWarpPosition));
  entityTemplate_->Set(
      String::New("destroy"),
      FunctionTemplate::New(entityDestroy));

  entityTemplate_->Set(
      String::New("getNearbyEntities"),
      FunctionTemplate::New(entityGetNearbyEntities));
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
      String::New("onTookDamage"),
      FunctionTemplate::New(entityOnTookDamage));
  
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
      LOG(ERROR) << "Unable to compile script '" << filename << "': " << *e_str << '\n';
    }

    Handle<Value> result = script->Run();
    if (result.IsEmpty()) {
      String::AsciiValue e_str(try_catch.Exception());
      LOG(ERROR) << "Unable to run script '" << filename << "': " << *e_str << '\n';
    }
  }
}

Handle<Object> GameScript::getEntity(id_t eid) {
  auto it = scriptObjects_.find(eid);
  invariant(it != scriptObjects_.end(), "no wrapper found for entity");

  return it->second;
}

Handle<Object> GameScript::getGlobal() {
  return context_->Global();
}

void GameScript::wrapEntity(GameEntity *e, const Json::Value &params) {
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  Persistent<Object> wrapper = Persistent<Object>::New(
      isolate_, entityTemplate_->NewInstance());
  wrapper->SetInternalField(0, External::New(e));

  const int argc = 2;
  Handle<Value> argv[argc] = {wrapper, jsonToJS(params)};
  Handle<Function>::Cast(context_->Global()->Get(String::New("entityInit")))
    ->Call(context_->Global(), argc, argv);

  scriptObjects_[e->getID()] = wrapper;
}

void GameScript::destroyEntity(id_t id) {
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  auto it = scriptObjects_.find(id);
  invariant(it != scriptObjects_.end(), "destroying entity without wrapper");
  it->second.Dispose();
  scriptObjects_.erase(it);
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
    return js->IntegerValue();
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
      << arr->Length() << " to vec2\n";
    return ret;
  }

  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  ret.z = arr->Get(2)->NumberValue();
  return ret;
}
};  // rts
