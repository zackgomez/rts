#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(
    id_t id,
    const std::string &name,
    const Json::Value &params)
  : ModelEntity(id),
    playerID_(NO_PLAYER),
    name_(name),
    maxSpeed_(0.f),
    sight_(0.f),
    warp_(false),
    uiInfo_() {
  setProperty(P_RENDERABLE, true);

  if (params.isMember("pos")) {
    setPosition(glm::vec3(toVec2(params["pos"]), 0.f));
  }
  if (params.isMember("angle")) {
    setAngle(params["angle"].asFloat());
  }

  resetTexture();
}

GameEntity::~GameEntity() {
}

void GameEntity::resetTexture() {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  auto color = player ? player->getColor() : ::vec3Param("global.defaultColor");
  setColor(color);
}

Clock::time_point GameEntity::getLastTookDamage(uint32_t part) const {
  auto it = lastTookDamage_.find(part);
  if (it != lastTookDamage_.end()) {
    return it->second;
  }
  return Clock::time_point();
}

id_t GameEntity::getTeamID() const {
  // No player, no team
  if (playerID_ == NO_PLAYER) {
    return NO_TEAM;
  }
  // A bit inefficient but OK for now
  return Game::get()->getPlayer(playerID_)->getTeamID();
}

float GameEntity::distanceToEntity(const GameEntity *e) const {
  return e->distanceFromPoint(getPosition2());
}

void GameEntity::setPlayerID(id_t pid) {
  assertPid(pid);
  playerID_ = pid;

  resetTexture();
}

void GameEntity::setTookDamage(int part_idx) {
  lastTookDamage_[part_idx] = Clock::now();
}

void GameEntity::remainStationary() {
  warp_ = false;
  pathQueue_ = std::vector<glm::vec3>();
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  setSpeed(0.f);
}

void GameEntity::turnTowards(const glm::vec2 &targetPos) {
  warp_ = false;
  setAngle(angleToTarget(targetPos));
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  setSpeed(0.f);
}

void GameEntity::moveTowards(const glm::vec2 &targetPos) {
  warp_ = false;
  if (hasProperty(P_COLLIDABLE)) {
    if (targetPos != lastTargetPos_) {
      pathQueue_ = Game::get()->getMap()->getNavMesh()->getPath(
          getPosition(),
          glm::vec3(targetPos, 0));
      lastTargetPos_ = targetPos;
    }
  } else {
    pathQueue_.clear();
    pathQueue_.push_back(glm::vec3(targetPos, 0.f));
  }
}

void GameEntity::warpPosition(const glm::vec2 &pos) {
  warp_ = true;
  warpTarget_ = pos;
  setSpeed(0.f);
  lastTargetPos_ = glm::vec2(HUGE_VAL);
  pathQueue_ = std::vector<glm::vec3>();
}

void GameEntity::resolve(float dt) {
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

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum
    .process(id)
    .process(playerID_)
    .process(name_)
    .process(getPosition())
    .process(getAngle())
    .process(getSize())
    .process(getHeight())
    .process(getSpeed());
}

std::vector<glm::vec3> GameEntity::getPathNodes() const {
  return pathQueue_;
}

void GameEntity::collide(const GameEntity *collider, float dt) {
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

void GameEntity::updateUIInfo() {
  uiInfo_ = GameEntity::UIInfo();

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {Integer::New(getID())};

  Handle<Value> ret =
    Handle<Function>::Cast(global->Get(String::New("entityGetUIInfo")))
    ->Call(global, argc, argv);
  checkJSResult(ret, try_catch, "entityGetUIInfo:");

  auto jsinfo = ret->ToObject();

  auto pid_str = String::New("pid");
  invariant(jsinfo->Has(pid_str), "UIInfo must have pid");
  setPlayerID(jsinfo->Get(pid_str)->IntegerValue());

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
    LOG(DEBUG) << "has hotkey " << hotkey_str << '\n';
    if (!hotkey_str.empty()) {
      invariant(hotkey_str.size() == 1, "expected single character hotkey string");
      uiInfo_.hotkey = hotkey_str[0];
      invariant(isControlGroupHotkey(uiInfo_.hotkey), "bad hotkey in uiinfo");
    }
    auto *player = Game::get()->getPlayer(getPlayerID());
    if (player && player->isLocal()) {
      LOG(DEBUG) << "setting hotkey " << hotkey_str << '\n';
      std::set<id_t> sel;
      sel.insert(getID());
      auto *lp = (LocalPlayer *)player;
      lp->addSavedSelection(hotkey_str[0], sel);
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

void GameEntity::updateActions() {
  actions_.clear();

  using namespace v8;
  auto script = Game::get()->getScript();
  HandleScope scope(script->getIsolate());
  TryCatch try_catch;
  auto global = script->getGlobal();
  const int argc = 1;
  Handle<Value> argv[argc] = {Integer::New(getID())};

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
