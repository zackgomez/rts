#include "rts/Game.h"
#include <algorithm>
#include <sstream>
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/GameEntity.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Renderer.h"

namespace rts {

Game* Game::instance_ = nullptr;

Game::Game(Map *map, const std::vector<Player *> &players, RenderProvider render_provider, ActionFunc action_func)
  : map_(map),
    players_(players),
    renderProvider_(render_provider),
    actionFunc_(action_func),
    running_(false),
    elapsedTime_(0.f) {
  std::set<int> team_ids;
  for (auto player : players) {
    player->setGame(this);
    requisition_[player->getPlayerID()] = 0.f;
    team_ids.insert(player->getTeamID());
  }
  for (int tid : team_ids) {
    victoryPoints_[tid] = 0.f;
  }
  // sort players to ensure consistency
  std::sort(
      players_.begin(), players_.end(),
      [](Player *p1, Player *p2) -> bool {
        return p1->getPlayerID() < p2->getPlayerID();
      });
  invariant(
      players_.size() <= map_->getMaxPlayers(),
      "too many players for map");

  assert(!instance_);
  instance_ = this;
}

Game::~Game() {
  delete map_;
  instance_ = nullptr;

}
  
UIAction UIActionFromJSON(const Json::Value &v) {
  UIAction uiaction;
  uiaction.name = must_have_idx(v, "name").asString();
  uiaction.icon = must_have_idx(v, "icon").asString();
  auto&& hotkey_str = must_have_idx(v, "hotkey").asString();
  uiaction.hotkey = !hotkey_str.empty() ? hotkey_str[0] : '\0';
  uiaction.tooltip = must_have_idx(v, "tooltip").asString();
  uiaction.targeting = static_cast<UIAction::TargetingType>(
      must_have_idx(v, "targeting").asInt());
  uiaction.range = must_have_idx(v, "range").asFloat();
  uiaction.radius = must_have_idx(v, "radius").asFloat();
  uiaction.state = static_cast<UIAction::ActionState>(
      must_have_idx(v, "state").asInt());
  uiaction.cooldown = must_have_idx(v, "cooldown").asFloat();

  return uiaction;
}

GameEntity::UIPart UIPartFromJSON(const Json::Value &v) {
  GameEntity::UIPart ret;
  ret.health = toVec2(must_have_idx(v, "health"));
  ret.name = must_have_idx(v, "name").asString();
  ret.tooltip = must_have_idx(v, "tooltip").asString();
  for (auto &&json_upgrade : must_have_idx(v, "upgrades")) {
    GameEntity::UIPartUpgrade upgrade;
    upgrade.name = must_have_idx(json_upgrade, "name").asString();
    upgrade.part = ret.name;
    ret.upgrades.push_back(upgrade);
  }
  return ret;
}

GameEntity::UIInfo UIInfoFromJSON(const Json::Value &v) {
  GameEntity::UIInfo ret;
  if (v.isMember("minimap_icon")) {
    ret.minimap_icon = v["minimap_icon"].asString();
  }
  if (v.isMember("mana")) {
    ret.mana = toVec2(v["mana"]);
  }
  if (v.isMember("retreat")) {
    ret.retreat = v["retreat"].asBool();
  }
  if (v.isMember("capture")) {
    ret.capture = toVec2(v["capture"]);
  }
  if (v.isMember("capture_pid")) {
    ret.capture_pid = toID(v["capture_pid"]);
  }
  if (v.isMember("path")) {
    for (auto &&pt : v["path"]) {
      ret.path.push_back(glm::vec3(toVec2(pt), 0.));
    }
  }
  if (v.isMember("parts")) {
    for (auto &&json_part : v["parts"]) {
      ret.parts.push_back(UIPartFromJSON(json_part));
    }
  }
  if (v.isMember("hotkey")) {
    auto&& hotkeystr = v["hotkey"].asString();
    ret.hotkey = hotkeystr.empty() ? '\0' : hotkeystr[0];
  }
  if (v.isMember("extra")) {
    ret.extra = v["extra"];
  }
  return ret;
}

void renderEntityFromJSON(GameEntity *e, const Json::Value &v) {
  invariant(e, "must have entity to render to");
  if (v.isMember("alive")) {
    for (auto &sample : v["alive"]) {
      e->setAlive(sample[0].asFloat(), sample[1].asBool());
    }
  }
  if (v.isMember("model")) {
    for (auto &sample : v["model"]) {
      e->setModelName(sample[1].asString());
    }
  }
  if (v.isMember("properties")) {
    for (auto &sample : v["properties"]) {
      for (auto &prop : sample[1]) {
        e->addProperty(prop.asInt());
      }
    }
  }
  if (v.isMember("pid")) {
    for (auto &sample : v["pid"]) {
      e->setPlayerID(sample[0].asFloat(), toID(sample[1]));
    }
  }
  if (v.isMember("tid")) {
    for (auto &sample : v["tid"]) {
      e->setTeamID(sample[0].asFloat(), toID(sample[1]));
    }
  }
  if (v.isMember("pos")) {
    for (auto &sample : v["pos"]) {
      e->setPosition(sample[0].asFloat(), toVec2(sample[1]));
    }
  }
  if (v.isMember("size")) {
    for (auto &sample : v["size"]) {
      e->setSize(sample[0].asFloat(), toVec3(sample[1]));
    }
  }
  if (v.isMember("angle")) {
    for (auto &sample : v["angle"]) {
      e->setAngle(sample[0].asFloat(), sample[1].asFloat());
    }
  }
  if (v.isMember("sight")) {
    for (auto &sample : v["sight"]) {
      e->setSight(sample[0].asFloat(), sample[1].asFloat());
    }
  }
  if (v.isMember("visible")) {
    for (auto &sample : v["visible"]) {
      float t = sample[0].asFloat();
      VisibilitySet set;
      for (auto pid : sample[1]) {
        set.insert(toID(pid));
      }
      e->setVisibilitySet(t, set);
    }
  }
  if (v.isMember("actions")) {
    for (auto &sample : v["actions"]) {
      float t = sample[0].asFloat();

      std::vector<UIAction> actions;
      for (auto &action_json : sample[1]) {
        auto uiaction = UIActionFromJSON(action_json);
        uiaction.owner_id = e->getGameID();
        actions.push_back(uiaction);
      }
      if (!actions.empty()) {
        e->setActions(t, actions);
      }
    }
  }
  if (v.isMember("ui_info")) {
    for (auto &&sample : v["ui_info"]) {
      const float t = sample[0].asFloat();
      GameEntity::UIInfo uiinfo = UIInfoFromJSON(sample[1]);
      e->setUIInfo(t, uiinfo);
    }
  }
}

void Game::handleRenderMessage(const Json::Value &v) {
  auto entities = must_have_idx(v, "entities");
  invariant(entities.isObject(), "should have entities array");
  auto entity_keys = entities.getMemberNames();
  for (auto &eid : entity_keys) {
    // find or create GameEntity corresponding to the
    auto it = game_to_render_id.find(eid);
    GameEntity *entity = nullptr;
    if (it == game_to_render_id.end()) {
      id_t new_id = Renderer::get()->newEntityID();
      auto game_entity = new GameEntity(new_id);
      game_entity->setGameID(eid);
      Renderer::get()->spawnEntity(game_entity);
      game_to_render_id[eid] = new_id;
      entity = game_entity;
    } else {
      entity = GameEntity::cast(Renderer::get()->getEntity(it->second));
    }
    renderEntityFromJSON(entity, entities[eid]);
  }

  auto events = must_have_idx(v, "events");
  invariant(events.isArray(), "events must be array");
  for (auto&& event : events) {
    add_effect(
        must_have_idx(event, "name").asString(),
        must_have_idx(event, "params"));
  }

  auto players = must_have_idx(v, "players");
  invariant(players.isArray(), "players must be array");
  for (int i = 0; i < players.size(); i++) {
    auto player = players[i];
    id_t pid = toID(must_have_idx(player, "pid"));
    requisition_[pid] = must_have_idx(player, "req").asFloat();
  }

  auto teams = must_have_idx(v, "teams");
  for (int i = 0; i < teams.size(); i++) {
    auto team = teams[i];
    id_t tid = toID(must_have_idx(team, "tid"));
    victoryPoints_[tid] = must_have_idx(team, "vps").asFloat();
  }
}

void Game::renderFromJSON(const Json::Value &msgs) {
  invariant(msgs.isArray(), "json messages must be array");
  invariant(msgs.size() > 0, "messges must be nonempty");
  for (auto msg : msgs) {
    auto type = must_have_idx(msg, "type").asString();
    if (type == "render") {
      handleRenderMessage(msg);
    } else if (type == "start") {
      Renderer::get()->setGameTime(0.f);
      Renderer::get()->setTimeMultiplier(1.f);
    } else if (type == "game_over") {
      // TODO(zack/connor): do more here
      auto winning_team = toID(must_have_idx(msg, "winning_team"));
      LOG(DEBUG) << "Winning team : " << winning_team << '\n';
      running_ = false;
    } else {
      invariant_violation("unknown message type: " + type);
    }
  }
}

void Game::addAction(id_t pid, const Json::Value &v) {
  auto act = v;
  act["from_pid"] = toJson(pid);
  actionFunc_(act);
}

void Game::run() {
  running_ = true;
  while (running_) {
    auto messages = renderProvider_();

    auto engine_lock = Renderer::get()->lockEngine();
    renderFromJSON(messages);
  }
}

GameEntity * Game::getEntity(const std::string &game_id) {
  auto it = game_to_render_id.find(game_id);
  if (it == game_to_render_id.end()) {
    return nullptr;
  }
  return GameEntity::cast(Renderer::get()->getEntity(it->second));
}

const GameEntity * Game::getEntity(const std::string &game_id) const {
  auto it = game_to_render_id.find(game_id);
  if (it == game_to_render_id.end()) {
    return nullptr;
  }
  return GameEntity::cast(Renderer::get()->getEntity(it->second));
}

const Player * Game::getPlayer(id_t pid) const {
  if (pid == NO_PLAYER) {
    return nullptr;
  }

  assertPid(pid);
  for (auto player : players_) {
    if (player->getPlayerID() == pid) {
      return player;
    }
  }

  invariant(false, "Asked to find unknown pid!");
  return nullptr;
}

float Game::getRequisition(id_t pid) const {
  auto it = requisition_.find(pid);
  invariant(it != requisition_.end(),
      "Unknown playerID to get requisition for");
  return it->second;
}

float Game::getVictoryPoints(id_t tid) const {
  auto it = victoryPoints_.find(assertTid(tid));
  invariant(it != victoryPoints_.end(),
      "Unknown teamID to get victory points for");
  return it->second;
}
};  // rts
