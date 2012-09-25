#ifndef SRC_COMMON_COLLISION_H_
#define SRC_COMMON_COLLISION_H_

#include <glm/glm.hpp>

bool pointInBox(
    const glm::vec2 &p,
    // boxPos is the center of the box
    const glm::vec2 &boxPos,
    const glm::vec2 &boxSize,
    float angle);

bool boxInBox(
    const glm::vec2 &pos1,
    glm::vec2 size1,
    float angle1,
    const glm::vec2 &pos2,
    glm::vec2 size2,
    float angle2);

bool boxInBoxOld(
    const glm::vec2 &pos1,
    const glm::vec2 &size1,
    float angle1,
    const glm::vec2 &pos2,
    const glm::vec2 &size2,
    float angle2);

#endif  // SRC_COMMON_COLLISION_H_
