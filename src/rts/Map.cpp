#include "rts/Map.h"
#include <vector>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/CollisionObject.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/Unit.h"

namespace rts {

Map::Map(const Json::Value &definition)
  : definition_(definition) {
}

glm::vec2 Map::getSize() const {
  return toVec2(definition_["size"]);
}

glm::vec4 Map::getColor() const {
  return toVec4(definition_["color"]);
}

float Map::getMapHeight(const glm::vec2 &pos) const {
  // TODO(zack): maps that have dynamic terrain will need to override this
  // to return the height at the passed in position
  return definition_["height"].asFloat();
}

void Map::update(float dt) {
  // nop
}

void Map::init(const std::vector<Player *> &players) {
  invariant(players.size() <= definition_["players"].asInt(),
      "too many players for map");

  Renderer::get()->setMapSize(getSize());
  Renderer::get()->setMapColor(getColor());

  Json::Value entities = definition_["entities"];
  for (int i = 0; i < entities.size(); i++) {
    Json::Value entity_def = entities[i];
    std::string type = entity_def["type"].asString();
    if (type == "starting_location") {
      spawnStartingLocation(entity_def, players);
      continue;
    }
    Json::Value params;
    params["entity_pid"] = toJson(NO_PLAYER);
    if (entity_def.isMember("pos")) {
      params["entity_pos"] = entity_def["pos"];
    }
    if (entity_def.isMember("size")) {
      params["entity_size"] = entity_def["size"];
    }
    if (entity_def.isMember("angle")) {
      params["entity_angle"] = entity_def["angle"];
    }
    invariant(entity_def.isMember("type"), "missing type param");
    Game::get()->spawnEntity(type, params);
  }
}

void Map::spawnStartingLocation(const Json::Value &definition,
    const std::vector<Player *> players) {
  invariant(definition.isMember("player"),
      "missing player for starting pos defintion");
  auto player = players[definition["player"].asInt() - 1];
  id_t pid = player->getPlayerID();

  glm::vec2 pos = toVec2(definition["pos"]);
  float angle = definition["angle"].asFloat();

  // Spawn starting building
  Json::Value params;
  params["entity_pid"] = toJson(pid);
  params["entity_pos"] = toJson(pos);
  params["entity_angle"] = angle;
  const GameEntity *base = Game::get()->spawnEntity("building", params);
  player->setBaseID(base->getID());

  float radians = deg2rad(angle);
  glm::vec2 dir(cos(radians), sin(radians));
  glm::vec2 tangent(dir.y, dir.x);

  // Starting units
  // TODO(zack): make this part of the definition
  pos += dir * 4.0f;
  pos -= tangent * 1.5f;
  for (int i = 0; i < 3; i++) {
    Json::Value params;
    params["entity_pid"] = toJson(pid);
    params["entity_pos"] = toJson(pos);
    params["entity_angle"] = angle;
    Game::get()->spawnEntity("melee_unit", params);
    pos += tangent * 1.5f;
  }
}
};  // rts
