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

static Handle<Value> entitySetPosition2(const Arguments &args) {
  invariant(args.Length() == 1, "void setPosition2(vec2 p)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  auto js_pos = Handle<Array>::Cast(args[0]);
  invariant(js_pos->Length() == 2, "expected vec2");
  glm::vec2 new_pos(
      js_pos->Get(Integer::New(0))->NumberValue(),
      js_pos->Get(Integer::New(1))->NumberValue());
  e->setPosition(new_pos);

  return Undefined();
}

static Handle<Value> entityGetID(const Arguments &args) {
  invariant(args.Length() == 0, "int getID()");
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  return scope.Close(Integer::New(e->getID()));
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

static Handle<Value> entitySetActions(const Arguments &args) {
  invariant(args.Length() == 1, "void setAction(array actions)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());

  auto name = String::New("name");
  auto icon = String::New("icon");
  auto hotkey = String::New("hotkey");
  auto tooltip = String::New("tooltip");
  auto targeting = String::New("targeting");
  auto range = String::New("range");
  auto radius = String::New("radius");
  auto state = String::New("state");
  auto cooldown = String::New("cooldown");

  auto jsactions = Handle<Array>::Cast(args[0]);
  std::vector<UIAction> actions;
  for (int i = 0; i < jsactions->Length(); i++) {
    Handle<Object> jsaction = Handle<Object>::Cast(jsactions->Get(i));
    UIAction uiaction;
    uiaction.owner = e->getID();
    uiaction.name = *String::AsciiValue(jsaction->Get(name));
    uiaction.icon = *String::AsciiValue(jsaction->Get(icon));
    std::string hotkey_str = *String::AsciiValue(jsaction->Get(hotkey));
    uiaction.hotkey = !hotkey_str.empty() ? hotkey_str[0] : '\0';
    uiaction.tooltip = *String::AsciiValue(jsaction->Get(tooltip));
    uiaction.targeting = static_cast<UIAction::TargetingType>(
        jsaction->Get(targeting)->IntegerValue());
    uiaction.range = jsaction->Get(range)->NumberValue();
    uiaction.radius = jsaction->Get(radius)->NumberValue();
    uiaction.state = static_cast<UIAction::ActionState>(
        jsaction->Get(state)->Uint32Value());
    uiaction.cooldown = jsaction->Get(cooldown)->NumberValue();
    actions.push_back(uiaction);
  }

  e->setActions(actions);

  return Undefined();
}

static Handle<Value> entitySetUIInfo(const Arguments &args) {
  invariant(args.Length() == 1, "void setUIInfo(object ui_info)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());
  auto script = Game::get()->getScript();

  auto jsinfo = args[0]->ToObject();

  auto ui_info = GameEntity::UIInfo();
  auto pid_str = String::New("pid");
  invariant(jsinfo->Has(pid_str), "UIInfo must have pid");
  e->setPlayerID(jsinfo->Get(pid_str)->IntegerValue());

  auto parts_str = String::New("parts");
  if (jsinfo->Has(parts_str)) {
    auto parts = Handle<Array>::Cast(jsinfo->Get(parts_str));
    for (int i = 0; i < parts->Length(); i++) {
      auto jspart = Handle<Object>::Cast(parts->Get(i));
      GameEntity::UIPart part;
      part.health = script->jsToVec2(
          Handle<Array>::Cast(jspart->Get(String::New("health"))));
      part.tooltip = *String::AsciiValue(
          jspart->Get(String::New("tooltip")));
      part.name = *String::AsciiValue(
          jspart->Get(String::New("name")));
      auto jsupgrades = Handle<Array>::Cast(
          jspart->Get(String::New("upgrades")));
      for (int j = 0; j < jsupgrades->Length(); j++) {
        auto jsupgrade = Handle<Object>::Cast(jsupgrades->Get(j));
        GameEntity::UIPartUpgrade upgrade;
        upgrade.part = part.name;
        upgrade.name = *String::AsciiValue(jsupgrade->Get(String::New("name")));
        part.upgrades.push_back(upgrade);
      }
      ui_info.parts.push_back(part);
    }
  }
  auto mana = String::New("mana");
  if (jsinfo->Has(mana)) {
    ui_info.mana = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(mana)));
  }
  auto capture = String::New("capture");
  if (jsinfo->Has(capture)) {
    ui_info.capture = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(capture)));
  }
  auto cap_pid = String::New("cappingPlayerID");
  ui_info.capture_pid = 0;
  if (jsinfo->Has(cap_pid)) {
    ui_info.capture_pid = jsinfo->Get(cap_pid)->IntegerValue();
  }
  auto retreat = String::New("retreat");
  ui_info.retreat = false;
  if (jsinfo->Has(retreat)) {
    ui_info.retreat = jsinfo->Get(retreat)->BooleanValue();
  }
  auto hotkey = String::New("hotkey");
  if (jsinfo->Has(hotkey)) {
    std::string hotkey_str = *String::AsciiValue(jsinfo->Get(hotkey));
    if (!hotkey_str.empty()) {
      invariant(hotkey_str.size() == 1, "expected single character hotkey string");
      ui_info.hotkey = hotkey_str[0];
      invariant(isControlGroupHotkey(ui_info.hotkey), "bad hotkey in uiinfo");
    }
    auto *player = Game::get()->getPlayer(e->getPlayerID());
    if (player && player->isLocal()) {
      std::set<id_t> sel;
      sel.insert(e->getID());
      auto *lp = (LocalPlayer *)player;
      lp->addSavedSelection(hotkey_str[0], sel);
    }
  }
  auto minimap_icon = String::New("minimap_icon");
  if (jsinfo->Has(minimap_icon)) {
    ui_info.minimap_icon = *String::AsciiValue(jsinfo->Get(minimap_icon));
  }

  auto extra = String::New("extra");
  invariant(jsinfo->Has(extra), "UIInfo should have extra map");
  ui_info.extra = script->jsToJSON(jsinfo->Get(extra));

  e->setUIInfo(ui_info);

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
      String::New("getID"),
      FunctionTemplate::New(entityGetID));

  entityTemplate_->Set(
      String::New("setModel"),
      FunctionTemplate::New(entitySetModel));
  entityTemplate_->Set(
      String::New("setPosition2"),
      FunctionTemplate::New(entitySetPosition2));
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
      String::New("setActions"),
      FunctionTemplate::New(entitySetActions));
  entityTemplate_->Set(
      String::New("setUIInfo"),
      FunctionTemplate::New(entitySetUIInfo));

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
