#include "rts/Map.h"
#include <vector>
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"

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
    invariant(entity_def.isMember("type"), "missing type param");
    std::string type = entity_def["type"].asString();
    if (type == "starting_location") {
      spawnStartingLocation(entity_def, players);
      continue;
    }
    if (type == "collision_object") {
      id_t eid = Renderer::get()->newEntityID();
      ModelEntity *obj = new ModelEntity(eid);
      obj->setPosition(glm::vec3(toVec2(entity_def["pos"]), 0.1f));
      obj->setSize(toVec2(entity_def["size"]));
      obj->setAngle(entity_def["angle"].asFloat());

      obj->setScale(glm::vec3(2.f*obj->getSize(), 1.f));
      GLuint texture = ResourceManager::get()->getTexture("collision-tex");
      obj->setMeshName("square");
      obj->setMaterial(createMaterial(glm::vec3(0.f), 0.f, texture));
      Renderer::get()->spawnEntity(obj);
      continue;
    }

    Json::Value params;
    params["pid"] = toJson(NO_PLAYER);
    if (entity_def.isMember("pos")) {
      params["pos"] = entity_def["pos"];
    }
    if (entity_def.isMember("size")) {
      params["size"] = entity_def["size"];
    }
    if (entity_def.isMember("angle")) {
      params["angle"] = entity_def["angle"];
    }
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
  params["pid"] = toJson(pid);
  params["pos"] = toJson(pos);
  params["angle"] = angle;
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
    params["pid"] = toJson(pid);
    params["pos"] = toJson(pos);
    params["angle"] = angle;
    Game::get()->spawnEntity("melee_unit", params);
    pos += tangent * 1.5f;
  }
}
};  // rts
