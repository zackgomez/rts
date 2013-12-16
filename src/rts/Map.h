#ifndef SRC_RTS_MAP_H_
#define SRC_RTS_MAP_H_

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/util.h"

namespace rts {

class Player;

class Map {
 public:
  explicit Map(const Json::Value &definition);

  glm::vec2 getSize() const;
  glm::vec4 getColor() const;

  // Returns the height of the map at the passed position.  This is currently
  // only used for cosmetic purposes
  float getMapHeight(const glm::vec2 &pos) const;
  size_t getMaxPlayers() const;

  // @param location_idx int in [0, max_players)
  // @return starting location defintion with keys pos -> vec2, angle -> float
  Json::Value getStartingLocation(int location_idx) const;
  // @return map definition for javascript consumption
  Json::Value getMapDefinition() const;

 private:
  Json::Value definition_;
};
};  // rts

#endif  // SRC_RTS_MAP_H_
