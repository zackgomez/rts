#include "rts/Map.h"
#include <vector>
#include "common/Collision.h"
#include "rts/Game.h"
#include "rts/Player.h"
#include "rts/Renderer.h"

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

size_t Map::getMaxPlayers() const {
  return must_have_idx(definition_, "players").asUInt();
}

void Map::init() {
  Renderer::get()->setMapSize(getSize());
  Renderer::get()->setMapColor(getColor());
  std::vector<std::tuple<glm::vec2, glm::vec2>> unpathable;

  Json::Value collision_objects =
    must_have_idx(definition_, "collision_objects");
  for (int i = 0; i < collision_objects.size(); i++) {
    Json::Value collision_object_def = collision_objects[i];

    id_t eid = Renderer::get()->newEntityID();
    glm::vec2 pos = toVec2(collision_object_def["pos"]);
    glm::vec2 size = toVec2(collision_object_def["size"]);
    ModelEntity *obj = new ModelEntity(eid);
    obj->setSize(size);
    obj->setPosition(0.f, glm::vec3(pos, 0.1f));
    obj->setAngle(0.f, collision_object_def["angle"].asFloat());

    unpathable.push_back(std::make_tuple(pos, size));

    obj->setScale(glm::vec3(2.f*obj->getSize(), 1.f));
    // TODO(zack): could be prettier than just a square
    obj->setModelName("square");
    Renderer::get()->spawnEntity(obj);
  }

  std::vector<std::vector<glm::vec3>> navFaces;
  const float cell_size = 2.f;
  auto extent = getSize() / 2.f;
  for (float x = -extent.x; x <= extent.x; x += cell_size) {
    for (float y = -extent.y; y <= extent.y; y += cell_size) {
      glm::vec2 center = glm::vec2(x, y) + glm::vec2(cell_size / 2.f);
      bool hit = false;
      for (auto obj : unpathable) {
        if (pointInBox(center, std::get<0>(obj), std::get<1>(obj), 0.f)) {
          hit = true;
          break;
        }
      }
      if (!hit) {
        std::vector<glm::vec3> face;
        face.push_back(glm::vec3(x, y, 0.f));
        face.push_back(glm::vec3(x + cell_size, y, 0.f));
        face.push_back(glm::vec3(x + cell_size, y + cell_size, 0.f));
        face.push_back(glm::vec3(x, y + cell_size, 0.f));
        navFaces.push_back(face);
      }
    }
  }

  navmesh_ = new NavMesh(navFaces);
}

Json::Value Map::getMapDefinition() const {
  return definition_;
}

Json::Value Map::getStartingLocation(int location_idx) const {
  auto starting_locations_def = must_have_idx(definition_, "starting_locations");
  invariant(
      starting_locations_def.isArray(),
      "starting locations definition must be json array");
  invariant(
      location_idx < starting_locations_def.size(),
      "starting location index out of bounds");

  return starting_locations_def[location_idx];
}
};  // rts
