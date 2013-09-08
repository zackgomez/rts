#include "rts/Actor.h"
#include <v8.h>
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/ResourceManager.h"
#include "rts/Renderer.h"

namespace rts {

Actor::Actor(id_t id, const std::string &name, const Json::Value &params)
  : GameEntity(id, name, params) {
  resetUIInfo();
  resetTexture();
}

Actor::~Actor() {
}

Clock::time_point Actor::getLastTookDamage(uint32_t part) const {
  auto it = lastTookDamage_.find(part);
  if (it != lastTookDamage_.end()) {
    return it->second;
  }
  return Clock::time_point();
}

void Actor::setTookDamage(int part_idx) {
  lastTookDamage_[part_idx] = Clock::now();
}

const std::vector<UIAction> &Actor::getActions() const {
  return actions_;
}

void Actor::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  setColor(color);
}

void Actor::collide(const GameEntity *collider, float dt) {
  if (!hasProperty(P_MOBILE)) {
    return;
  }
  if (!collider->hasProperty(P_ACTOR)
      || collider->getPlayerID() != getPlayerID()) {
    return;
  }

  // if either entity is moving forward, just ignore the message
  if (getSpeed() != 0.f || collider->getSpeed() != 0.f) {
    return;
  }

  // get difference between positions at time of intersection
  auto diff = getPosition2(dt) - collider->getPosition2(dt);
  float overlap_dist = glm::length(diff);

  // seperate away, or randomly if exactly the same pos
  auto dir = (overlap_dist > 1e-6f)
    ? diff / overlap_dist
    : randDir2();

  float bumpSpeed = ::fltParam("global.bumpSpeed");
  addBumpVel(glm::vec3(dir * bumpSpeed, 0.f));
}

void Actor::resolve(float dt) {
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  Handle<Object> global = script->getGlobal();
  TryCatch try_catch;

  const int argc = 2;
  Handle<Value> argv[argc] = {script->getEntity(getID()), Number::New(dt)};
  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityResolve")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch, "entityResolve:");

	if (hasProperty(P_MOBILE) && warp_) {
		setPosition(warpTarget_);
	} else if (hasProperty(P_MOBILE) && !pathQueue_.empty() && getMaxSpeed() > 0) {
    glm::vec2 targetPos(pathQueue_.front());
    float dist = glm::length(targetPos - getPosition2());
    float speed = getMaxSpeed();
    // rotate
    turnTowards(targetPos);
    // move
    // Set speed careful not to overshoot
    if (dist < speed * dt) {
      speed = dist / dt;
      // 'pop'
      pathQueue_.erase(pathQueue_.begin());
    }
    setSpeed(speed);
  }

  updateUIInfo();
  updateActions();
}

void Actor::handleOrder(const Message &order) {
  if (order["type"] == "ACTION") {
    invariant(order.isMember("action"), "missing action name");
    std::string action_name = order["action"].asString();
    handleAction(action_name, order);
  } else {
    using namespace v8;
    auto *script = Game::get()->getScript();
    HandleScope scope(script->getIsolate());
    Handle<Object> global = script->getGlobal();
    TryCatch try_catch;

    // TODO(zack): HACK! either pass vec2 or expect vec3
    Json::Value cleanorder = order;
    if (order.isMember("target")) {
      cleanorder["target"] = toJson(toVec2(order["target"]));
    }

    Handle<Object> jsorder = Handle<Object>::Cast(script->jsonToJS(cleanorder));

    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(getID()), jsorder};
    Handle<Value> result =
      Handle<Function>::Cast(global->Get(String::New("entityHandleOrder")))
      ->Call(global, argc, argv);
    checkJSResult(result, try_catch, "entityHandleOrder:");
  }
}

void Actor::handleAction(const std::string &action_name, const Json::Value &order) {
  using namespace v8;
  auto *script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());

  Handle<Object> global = script->getGlobal();
  TryCatch try_catch;
  // TODO(zack): convert this to a params array rather than a single 'target'
  // variable.
  Handle<Object> action_args = Object::New();
  if (order.isMember("target")) {
    action_args->Set(
        String::New("target"),
        script->jsonToJS(order["target"]));
  } else if (order.isMember("target_id")) {
    action_args->Set(
        String::New("target_id"),
        script->jsonToJS(order["target_id"]));
  }

  const int argc = 3;
  Handle<Value> argv[argc] = {
    script->getEntity(getID()),
    String::New(action_name.c_str()),
    action_args,
  };
  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityHandleAction")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch, "entityHandleAction:");

  return;
}

void Actor::update(float dt) {
  GameEntity::update(dt);

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;

  auto global = script->getGlobal();
  const int argc = 2;

  Handle<Value> argv[] = {script->getEntity(getID()), Number::New(dt)};

  Handle<Value> result =
    Handle<Function>::Cast(global->Get(String::New("entityUpdate")))
    ->Call(global, argc, argv);
  checkJSResult(result, try_catch, "entityUpdate:");
}

void Actor::resetUIInfo() {
  uiInfo_ = Actor::UIInfo();
}

void Actor::updateUIInfo() {
  resetUIInfo();

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {script->getEntity(getID())};

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityGetUIInfo")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "entityGetUIInfo:");

  auto jsinfo = ret->ToObject();
  auto parts_str = String::New("parts");
  if (jsinfo->Has(parts_str)) {
    auto parts = Handle<Array>::Cast(jsinfo->Get(parts_str));
    for (int i = 0; i < parts->Length(); i++) {
      auto jspart = Handle<Object>::Cast(parts->Get(i));
      UIPart part;
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
        UIPartUpgrade upgrade;
        upgrade.part = part.name;
        upgrade.name = *String::AsciiValue(jsupgrade->Get(String::New("name")));
        part.upgrades.push_back(upgrade);
      }
      uiInfo_.parts.push_back(part);
    }
  }
  auto mana = String::New("mana");
  if (jsinfo->Has(mana)) {
    uiInfo_.mana = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(mana)));
  }
  auto capture = String::New("capture");
  if (jsinfo->Has(capture)) {
    uiInfo_.capture = script->jsToVec2(
        Handle<Array>::Cast(jsinfo->Get(capture)));
  }
  auto cap_pid = String::New("cappingPlayerID");
  uiInfo_.capture_pid = 0;
  if (jsinfo->Has(cap_pid)) {
    uiInfo_.capture_pid = jsinfo->Get(cap_pid)->IntegerValue();
  }
  auto retreat = String::New("retreat");
  uiInfo_.retreat = false;
  if (jsinfo->Has(retreat)) {
    uiInfo_.retreat = jsinfo->Get(retreat)->BooleanValue();
  }
  auto hotkey = String::New("hotkey");
  if (jsinfo->Has(hotkey)) {
    std::string hotkey_str = *String::AsciiValue(jsinfo->Get(hotkey));
    if (!hotkey_str.empty()) {
      invariant(hotkey_str.size() == 1, "expected single character hotkey string");
      uiInfo_.hotkey = hotkey_str[0];
      invariant(isControlGroupHotkey(uiInfo_.hotkey), "bad hotkey in uiinfo");
    }
  }
  auto minimap_icon = String::New("minimap_icon");
  if (jsinfo->Has(minimap_icon)) {
    uiInfo_.minimap_icon = *String::AsciiValue(jsinfo->Get(minimap_icon));
  }

  auto extra = String::New("extra");
  invariant(jsinfo->Has(extra), "UIInfo should have extra map");
  uiInfo_.extra = script->jsToJSON(jsinfo->Get(extra));
}

void Actor::updateActions() {
  actions_.clear();

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {script->getEntity(getID())};

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityGetActions")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "entityGetActions:");

  auto name = String::New("name");
  auto icon = String::New("icon");
  auto hotkey = String::New("hotkey");
  auto tooltip = String::New("tooltip");
  auto targeting = String::New("targeting");
  auto range = String::New("range");
  auto radius = String::New("radius");
  auto state = String::New("state");
  auto cooldown = String::New("cooldown");
  Handle<Array> jsactions = Handle<Array>::Cast(ret);
  for (int i = 0; i < jsactions->Length(); i++) {
    Handle<Object> jsaction = Handle<Object>::Cast(jsactions->Get(i));
    UIAction uiaction;
    uiaction.owner = getID();
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
    actions_.push_back(uiaction);
  }
}
};  // rts
