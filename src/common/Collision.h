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

float segmentLineIntersection(
  const glm::vec2 &start,
  const glm::vec2 &end,
  const glm::vec2 &l0,
  const glm::vec2 &l1);

// returns time of intersection or NO_INTERSECTION
// NOTE this will not return an intersection if the origin lies on
// a box plane
float rayBox2Intersection(
    const glm::vec2 &origin,
    const glm::vec2 &dir,
    const Rect &box);

float rayAABB2Intersection(
    const glm::vec2 &origin,
    const glm::vec2 &dir,
    const glm::vec2 &center,
    const glm::vec2 &size);


// returns time of intersection or NO_INTERSECTION
float rayAABBIntersection(
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    const glm::vec3 &center,
    const glm::vec3 &size);

// Returns the time of intersection in [0, dt] or NO_INTERESECTION
float boxBoxCollision(
    const Rect &r1,
    const glm::vec2 &v1,
    const Rect &r2,
    const glm::vec2 &v2,
    float dt);

#endif  // SRC_COMMON_COLLISION_H_
