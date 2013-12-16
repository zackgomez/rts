#include "rts/Map.h"
#include <vector>
#include "common/Collision.h"
#include "rts/Game.h"
#include "rts/ModelEntity.h"
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
