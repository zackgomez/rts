#ifndef SRC_RTS_MAP_H_
#define SRC_RTS_MAP_H_

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/util.h"
#include "common/NavMesh.h"

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

  // Initializes the map and any start entities/etc
  void init(const std::vector<Player *> &players);
  void update(float dt);
  
  NavMesh* getNavMesh() const { return navmesh_; }

 private:
  Json::Value definition_;

  void spawnStartingLocation(const Json::Value &definition,
    const std::vector<Player *> players);
  NavMesh *navmesh_;
};
};  // rts

#endif  // SRC_RTS_MAP_H_
