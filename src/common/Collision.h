#ifndef SRC_COMMON_COLLISION_H_
#define SRC_COMMON_COLLISION_H_

#include <glm/glm.hpp>
#include <vector>

struct Circle {
  glm::vec2 center;
  float radius;
};

struct Rect {
  Rect(const glm::vec2 &p, const glm::vec2 &s, float a) :
      pos(p), size(s), angle(a)
  { }
  glm::vec2 pos;
  glm::vec2 size;
  float angle;

  bool contains(const glm::vec2 &p) const;
};

bool pointInBox(
    const glm::vec2 &p,
    // boxPos is the center of the box
    const glm::vec2 &boxPos,
    const glm::vec2 &boxSize,
    float angle);

bool boxInBox(
    const Rect &r1,
    const Rect &r2);

bool pointInPolygon(const glm::vec3 &point,
    const std::vector<glm::vec3> &polygon);

const float NO_INTERSECTION = -1.f;

// Returns the time of intersection in [0, dt] or -1 if no intersection
float boxBoxCollision(
    const Rect &r1,
    const glm::vec2 &v1,
    const Rect &r2,
    const glm::vec2 &v2,
    float dt);

#endif  // SRC_COMMON_COLLISION_H_
