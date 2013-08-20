#ifndef SRC_RTS_GEOMETRY_H_
#define SRC_RTS_GEOMETRY_H_
#include <glm/glm.hpp>

//  assumes that the parallel faces are 'up' and 'down'
//    ___
//   /   \
//   \___/
enum class HexTiling {
  ABOVE,
  BELOW,
  HIGH_LEFT,
  LOW_LEFT,
  HIGH_RIGHT,
  LOW_RIGHT,
};

// Returns a static list of iterable hextilings, length = 6
const HexTiling * getHexTilings();

// returns the center of a hexagon at the specified tile position given
// an origin and size.  Optional margin is distance between edges.
glm::vec2 computeTiledHexPosition(
    HexTiling tile,
    const glm::vec2 &pos,
    const glm::vec2 &size,
    float margin = 0.f);

#endif  // SRC_RTS_GEOMETRY_H_
