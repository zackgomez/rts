#include "rts/GameScript.h"
#include <v8.h>
#include <sstream>
#include "common/Collision.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/GameController.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"

using namespace v8;

namespace rts {

void jsFail(const v8::TryCatch &try_catch, const std::string &msg) {
  LOG(ERROR) << msg << " "
    << *String::AsciiValue(try_catch.Exception()) << '\n'
    << *String::AsciiValue(try_catch.StackTrace()) << '\n';
  invariant_violation("Javascript exception");
}

static Handle<Value> runtimeEvalImpl(
    Handle<String> source,
    Handle<Object> kwargs);

static Handle<Value> runtimeEval(const Arguments &args) {
  if (args.Length() < 2) {
    invariant_violation("value runtimeEval(source, kwargs)");
  }
  HandleScope scope(args.GetIsolate());

  auto js_source = args[0]->ToString();
  auto js_kwargs = args[1]->ToObject();
  return scope.Close(runtimeEvalImpl(js_source, js_kwargs));
}

static Handle<Value> jsPointInOBB2(const Arguments &args);
static Handle<Value> getCollisionBinding() {
  HandleScope scope;
  auto binding = Object::New();
  binding->Set(
      String::New("pointInOBB2"),
      FunctionTemplate::New(jsPointInOBB2)->GetFunction());

  return scope.Close(binding);
}

static Handle<Value> jsResolveCollisions(const Arguments &args);
static Handle<Value> jsComputePath(const Arguments &args);
static Handle<Value> getPathingBinding() {
  HandleScope scope;
  auto binding = Object::New();
  binding->Set(
      String::New("resolveCollisions"),
      FunctionTemplate::New(jsResolveCollisions)->GetFunction());
  binding->Set(
      String::New("computePath"),
      FunctionTemplate::New(jsComputePath)->GetFunction());

  return scope.Close(binding);
}

static Handle<Value> jsRuntimeBinding(const Arguments &args) {
  invariant(args.Length() == 1, "value runtime.binding(string name)");
  HandleScope scope(args.GetIsolate());

  auto name = Handle<String>::Cast(args[0]);
  auto binding_map = Game::get()->getScript()->getBindings();
  invariant(binding_map->Has(args[0]), "unknown binding");
  return scope.Close(binding_map->Get(name));
}

static Handle<Value> runtimeSetGameObject(const Arguments &args) {
  invariant(args.Length() == 1, "void SetGameObject(object game)");

  HandleScope scope(args.GetIsolate());
  Game::get()->setJSGameObject(args[0]->ToObject());
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

static Handle<Value> jsPointInOBB2(const Arguments &args) {
  invariant(args.Length() == 4, "bool pointInOBB2(p, center, size, angle)");
  HandleScope scope(args.GetIsolate());
  auto script = Game::get()->getScript();

  auto pt = script->jsToVec2(Handle<Array>::Cast(args[0]));
  auto center = script->jsToVec2(Handle<Array>::Cast(args[1]));
  auto size = script->jsToVec2(Handle<Array>::Cast(args[2]));
  auto angle = args[3]->NumberValue();

  bool contains = pointInBox(pt, center, size, angle);
  return scope.Close(Boolean::New(contains));
}

static Handle<Value> jsResolveCollisions(const Arguments &args) {
  invariant(
      args.Length() == 3,
      "resolveCollisions(map<key, objecy> bodies, float dt, func callback): void");
  HandleScope scope(args.GetIsolate());
  auto script = Game::get()->getScript();
  auto global = script->getContext()->Global();

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
    auto pos1 = script->jsToVec2(Handle<Array>::Cast(body1->Get(pos_str)));
    auto size1 = script->jsToVec2(Handle<Array>::Cast(body1->Get(size_str)));
    auto angle1 = body1->Get(angle_str)->NumberValue();
    auto vel1 = script->jsToVec2(Handle<Array>::Cast(body1->Get(vel_str)));
    Rect rect1(pos1, size1, angle1);

    for (auto j = i+1; j < keys->Length(); j++) {
      auto key2 = keys->Get(j);
      auto body2 = Handle<Object>::Cast(bodies->Get(key2));
      auto pos2 = script->jsToVec2(Handle<Array>::Cast(body2->Get(pos_str)));
      auto size2 = script->jsToVec2(Handle<Array>::Cast(body2->Get(size_str)));
      auto angle2 = body2->Get(angle_str)->NumberValue();
      auto vel2 = script->jsToVec2(Handle<Array>::Cast(body2->Get(vel_str)));
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

  return Undefined();
}

static Handle<Value> jsComputePath(const Arguments &args) {
  invariant(
      args.Length() == 2,
      "computePath(vec2 start, vec2 end): array<vec2> path");
  HandleScope scope(args.GetIsolate());
  auto script = Game::get()->getScript();
  auto navmesh = Game::get()->getMap()->getNavMesh();

  glm::vec3 start(
      script->jsToVec2(Handle<Array>::Cast(args[0])),
      0.f);
  glm::vec3 end(
      script->jsToVec2(Handle<Array>::Cast(args[1])),
      0.f);

  auto path = navmesh->getPath(start, end);

  auto jspath = Array::New();
  for (const auto &node : path) {
    jspath->Set(jspath->Length(), script->vec2ToJS(glm::vec2(node)));
  }

  return scope.Close(jspath);
}

static Handle<Value> entitySetPosition2(const Arguments &args) {
  invariant(args.Length() == 2, "void setPosition2(float t, vec2 p)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  auto t = args[0]->NumberValue();
  auto js_pos = Handle<Array>::Cast(args[1]);
  invariant(js_pos->Length() == 2, "expected vec2");
  glm::vec2 new_pos(
      js_pos->Get(Integer::New(0))->NumberValue(),
      js_pos->Get(Integer::New(1))->NumberValue());
  glm::vec3 full_pos(
      new_pos,
      Game::get()->getMap()->getMapHeight(new_pos));
  e->setPosition(t, full_pos);

  return Undefined();
}

static Handle<Value> entitySetAngle(const Arguments &args) {
  invariant(args.Length() == 2, "void setAngle(float t, float a)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setAngle(args[0]->NumberValue(), args[1]->NumberValue());

  return Undefined();
}

static Handle<Value> entitySetVisible(const Arguments &args) {
  invariant(
      args.Length() == 2,
      "void setVisible(float t, pid[] visibility_set)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  float t = args[0]->NumberValue();
  auto jspids = Handle<Array>::Cast(args[1]);
  VisibilitySet set;
  for (int i = 0; i < jspids->Length(); i++) {
    id_t pid = jspids->Get(i)->IntegerValue();
    set.insert(pid);
  }
  e->setVisibilitySet(t, set);

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

static Handle<Value> entitySetGameID(const Arguments &args) {
  invariant(args.Length() == 1, "setGameID(string id)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setGameID(*String::AsciiValue(args[0]->ToString()));
  return Undefined();
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
  invariant(args.Length() == 1, "setSize takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  glm::vec2 size = Game::get()->getScript()->jsToVec2(
      Handle<Array>::Cast(args[0]));
  e->setSize(size);

  return Undefined();
}

static Handle<Value> entitySetHeight(const Arguments &args) {
  invariant(args.Length() == 1, "setHeight takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  float height = args[0]->NumberValue();
  e->setHeight(height);

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
    uiaction.render_id = e->getID();
    uiaction.owner_id = e->getGameID();
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
      std::set<std::string> sel;
      sel.insert(e->getGameID());
      auto *lp = (LocalPlayer *)player;
      lp->addSavedSelection(hotkey_str[0], sel);
    }
  }
  auto minimap_icon = String::New("minimap_icon");
  if (jsinfo->Has(minimap_icon)) {
    ui_info.minimap_icon = *String::AsciiValue(jsinfo->Get(minimap_icon));
  }
  auto path_str = String::New("path");
  if (jsinfo->Has(path_str)) {
    auto jspath = Handle<Array>::Cast(jsinfo->Get(path_str));
    for (auto i = 0; i < jspath->Length(); i++) {
      glm::vec3 node(
          script->jsToVec2(Handle<Array>::Cast(jspath->Get(i))),
          0.f);
      ui_info.path.push_back(node);
    }
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
    jsBindings_.Dispose();
    context_.Dispose(isolate_);
  }
  isolate_->Exit();
  isolate_->Dispose();
}

void configure_v8() {
  const int v8_argc = 4;
  int argc = v8_argc;
  char *argv[] = {
    NULL,
    strdup("--use_strict"),
    strdup("--harmony_array_buffer"),
    strdup("--harmony_typed_arrays"),
  };
  v8::V8::SetFlagsFromCommandLine(&argc, argv, false);
  for (int i = 0; i < v8_argc; i++) {
    free(argv[i]);
  }
}

void GameScript::init(const std::string &init_function_path) {
  configure_v8();

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
      String::New("AddEffect"),
      FunctionTemplate::New(jsAddEffect));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  // init bindings
  jsBindings_ = Persistent<Object>::New(isolate_, Object::New());
  jsBindings_->Set(
      String::New("collision"),
      getCollisionBinding());
  jsBindings_->Set(
      String::New("pathing"),
      getPathingBinding());

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
  runtime_object->Set(
      String::New("setGameObject"),
      FunctionTemplate::New(runtimeSetGameObject)->GetFunction());
  context_->Global()->Set(
      String::New("runtime"),
      runtime_object);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->Set(
      String::New("getID"),
      FunctionTemplate::New(entityGetID));

  entityTemplate_->Set(
      String::New("setGameID"),
      FunctionTemplate::New(entitySetGameID));
  entityTemplate_->Set(
      String::New("setModel"),
      FunctionTemplate::New(entitySetModel));
  entityTemplate_->Set(
      String::New("setPosition2"),
      FunctionTemplate::New(entitySetPosition2));
  entityTemplate_->Set(
      String::New("setAngle"),
      FunctionTemplate::New(entitySetAngle));
  entityTemplate_->Set(
      String::New("setVisible"),
      FunctionTemplate::New(entitySetVisible));
  entityTemplate_->Set(
      String::New("setSize"),
      FunctionTemplate::New(entitySetSize));
  entityTemplate_->Set(
      String::New("setHeight"),
      FunctionTemplate::New(entitySetHeight));
  entityTemplate_->Set(
      String::New("setSight"),
      FunctionTemplate::New(entitySetSight));

  entityTemplate_->Set(
      String::New("setProperties"),
      FunctionTemplate::New(entitySetProperties));
  entityTemplate_->Set(
      String::New("setActions"),
      FunctionTemplate::New(entitySetActions));
  entityTemplate_->Set(
      String::New("setUIInfo"),
      FunctionTemplate::New(entitySetUIInfo));

  std::ifstream init_file(init_function_path);
  std::string init_function_source;
  std::getline(init_file, init_function_source, (char)EOF);

  auto kwargs = Object::New();
  kwargs->Set(
      String::New("filename"),
      String::New(init_function_path.c_str()));

  auto init_function_val = runtimeEvalImpl(
      String::New(init_function_source.c_str()), 
      kwargs);
  invariant(
      init_function_val->IsFunction(),
      "init file must evaluate to a function");
  const int argc = 1;
  Handle<Value> argv[] = { runtime_object };
  Handle<Function>::Cast(init_function_val)->Call(getGlobal(), argc, argv);
}


Handle<Value> runtimeEvalImpl(Handle<String> source, Handle<Object> kwargs) {
  TryCatch try_catch;

  auto jsfilename = kwargs->Get(String::New("filename"));
  auto filename = *String::AsciiValue(jsfilename);
  LOG(DEBUG) << "Filename: " << filename << '\n';

  Handle<Script> script = Script::Compile(source, jsfilename);
  checkJSResult(script, try_catch, "compile:");

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
  invariant(
      arr->Length() == 2,
      "trying to convert an incorrectly sized array to vec2");

  glm::vec2 ret;
  ret.x = arr->Get(0)->NumberValue();
  ret.y = arr->Get(1)->NumberValue();
  return ret;
}
glm::vec3 GameScript::jsToVec3(const Handle<Array> arr) const {
  invariant(
      arr->Length() == 3,
      "trying to convert an incorrectly sized array to vec2");

  glm::vec3 ret;
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
