#include "rts/RendererBinding.h"
#include <v8.h>
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Renderer.h"

using namespace v8;
using namespace rts;

static Persistent<ObjectTemplate> entity_template;

static void entitySetPosition2(const FunctionCallbackInfo<Value> &args) {
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
  e->setPosition(t, new_pos);

  args.GetReturnValue().SetUndefined();
}

static void entitySetAngle(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 2, "void setAngle(float t, float a)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setAngle(args[0]->NumberValue(), args[1]->NumberValue());

  args.GetReturnValue().SetUndefined();
}

static void entitySetVisible(const FunctionCallbackInfo<Value> &args) {
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
    rts::id_t pid = jspids->Get(i)->IntegerValue();
    set.insert(pid);
  }
  e->setVisibilitySet(t, set);

  args.GetReturnValue().SetUndefined();
}

static void entityGetID(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 0, "int getID()");
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  args.GetReturnValue().Set(scope.Close(Integer::New(e->getID())));
}

static void entitySetGameID(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "setGameID(string id)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setGameID(*String::AsciiValue(args[0]->ToString()));
  args.GetReturnValue().SetUndefined();
}

static void entitySetModel(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "setModel(string model)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  e->setModelName(*String::AsciiValue(args[0]));
  args.GetReturnValue().SetUndefined();
}

static void entitySetSight(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "setSetSight takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());

  e->setSight(args[0]->NumberValue());
  args.GetReturnValue().SetUndefined();
}

static void entitySetSize(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "setSize takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  glm::vec2 size = jsToVec2(
      Handle<Array>::Cast(args[0]));
  e->setSize(size);

  args.GetReturnValue().SetUndefined();
}

static void entitySetHeight(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "setHeight takes 1 arg");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  float height = args[0]->NumberValue();
  e->setHeight(height);

  args.GetReturnValue().SetUndefined();
}

static void entitySetProperties(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "void entitySetProperties(array properties)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());

  auto js_properties = Handle<Array>::Cast(args[0]);
  for (int i = 0; i < js_properties->Length(); i++) {
    e->addProperty(js_properties->Get(Integer::New(i))->IntegerValue());
  }

  args.GetReturnValue().SetUndefined();
}

static void entitySetActions(const FunctionCallbackInfo<Value> &args) {
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

  args.GetReturnValue().SetUndefined();
}

static void entitySetUIInfo(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "void setUIInfo(object ui_info)");

  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  auto *e = static_cast<GameEntity *>(wrap->Value());

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
      part.health = jsToVec2(
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
    ui_info.mana = jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(mana)));
  }
  auto capture = String::New("capture");
  if (jsinfo->Has(capture)) {
    ui_info.capture = jsToVec2(
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
          jsToVec2(Handle<Array>::Cast(jspath->Get(i))),
          0.f);
      ui_info.path.push_back(node);
    }
  }

  auto extra = String::New("extra");
  invariant(jsinfo->Has(extra), "UIInfo should have extra map");
  ui_info.extra = jsToJSON(jsinfo->Get(extra));

  e->setUIInfo(ui_info);

  args.GetReturnValue().SetUndefined();
}

static Handle<ObjectTemplate> make_entity_template() {
  auto entity_template = ObjectTemplate::New();
  entity_template->SetInternalFieldCount(1);
  entity_template->Set(
      String::New("getID"),
      FunctionTemplate::New(entityGetID));

  entity_template->Set(
      String::New("setGameID"),
      FunctionTemplate::New(entitySetGameID));
  entity_template->Set(
      String::New("setModel"),
      FunctionTemplate::New(entitySetModel));
  entity_template->Set(
      String::New("setPosition2"),
      FunctionTemplate::New(entitySetPosition2));
  entity_template->Set(
      String::New("setAngle"),
      FunctionTemplate::New(entitySetAngle));
  entity_template->Set(
      String::New("setVisible"),
      FunctionTemplate::New(entitySetVisible));
  entity_template->Set(
      String::New("setSize"),
      FunctionTemplate::New(entitySetSize));
  entity_template->Set(
      String::New("setHeight"),
      FunctionTemplate::New(entitySetHeight));
  entity_template->Set(
      String::New("setSight"),
      FunctionTemplate::New(entitySetSight));

  entity_template->Set(
      String::New("setProperties"),
      FunctionTemplate::New(entitySetProperties));
  entity_template->Set(
      String::New("setActions"),
      FunctionTemplate::New(entitySetActions));
  entity_template->Set(
      String::New("setUIInfo"),
      FunctionTemplate::New(entitySetUIInfo));

  return entity_template;
}

static Handle<ObjectTemplate> get_entity_template() {
  invariant(!entity_template.IsEmpty(), "must have initialized entity template");
  return Handle<ObjectTemplate>::New(Isolate::GetCurrent(), entity_template);
}

static void jsSpawnRenderEntity(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 0, "object spawnRenderEntity(void)");
  HandleScope scope(args.GetIsolate());

  rts::id_t eid = Renderer::get()->newEntityID();
  GameEntity *e = new GameEntity(eid);
  invariant(e, "couldn't allocate new entity");
  Renderer::get()->spawnEntity(e);

  auto entity_template = get_entity_template();
  Handle<Object> wrapper = entity_template->NewInstance();
  wrapper->SetInternalField(0, External::New(e));

  args.GetReturnValue().Set(scope.Close(wrapper));
}

static void jsDestroyRenderEntity(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 1, "void DestroyRenderEntity(int eid");
  HandleScope scope(args.GetIsolate());

  rts::id_t eid = args[0]->IntegerValue();
  Renderer::get()->removeEntity(eid);

  args.GetReturnValue().SetUndefined();
}

static void jsAddEffect(const FunctionCallbackInfo<Value> &args) {
  invariant(args.Length() == 2, "jsAddEffect(name, params)");
  HandleScope scope(args.GetIsolate());

  std::string name = *String::AsciiValue(args[0]);
  auto params = Handle<Object>::Cast(args[1]);

  add_jseffect(name, params);

  args.GetReturnValue().SetUndefined();
}

Handle<Value> getRendererBinding() {
  HandleScope scope(Isolate::GetCurrent());
  entity_template.Reset(Isolate::GetCurrent(), make_entity_template());
  auto binding = Object::New();
  binding->Set(
      String::New("spawnRenderEntity"),
      FunctionTemplate::New(jsSpawnRenderEntity)->GetFunction());
  binding->Set(
      String::New("destroyRenderEntity"),
      FunctionTemplate::New(jsDestroyRenderEntity)->GetFunction());
  binding->Set(
      String::New("addEffect"),
      FunctionTemplate::New(jsAddEffect)->GetFunction());

  return scope.Close(binding);
}
