#include "common/Collision.h"
#include <algorithm>  // for min/max
#include "common/Logger.h"
#include "common/util.h"

bool Rect::contains(const glm::vec2 &p) const {
  return pointInBox(p, pos, size, angle);
}

bool pointInBox(
    const glm::vec2 &p,
    const glm::vec2 &boxPos,
    const glm::vec2 &boxSize,
    float angle) {
  // get the point relative to the entity
  glm::vec2 pt = p - boxPos;
  pt = glm::vec2(
      glm::dot(glm::vec2(glm::cos(angle), glm::sin(angle)), pt),
      glm::dot(glm::vec2(-glm::sin(angle), glm::cos(angle)), pt));
  return pt.x >= -boxSize.x / 2 && pt.x < boxSize.x / 2 &&
         pt.y >= -boxSize.y / 2 && pt.y < boxSize.y / 2;
}

bool boxInBox(
    const Rect &rect1,
    const Rect &rect2) {
  // Convert sizes to 'extents'
  const glm::vec2 size1 = rect1.size / 2.f;
  const glm::vec2 size2 = rect2.size / 2.f;
  // D = C_1 - C_0 where C_0 is center of first box
  const glm::vec2 D = rect2.pos - rect1.pos;
  // Columns of A/B correspond to the axis of the box
  const glm::mat2 A = glm::mat2(
      glm::vec2(glm::cos(rect1.angle), glm::sin(rect1.angle)),
      glm::vec2(-glm::sin(rect1.angle), glm::cos(rect1.angle)));
  const glm::mat2 B = glm::mat2(
      glm::vec2(glm::cos(rect2.angle), glm::sin(rect2.angle)),
      glm::vec2(-glm::sin(rect2.angle), glm::cos(rect2.angle)));
  // Each c_ij is A_i * B_j where A/B_k is an axis of that box
  const glm::mat2 C = glm::transpose(A) * B;
  // NOTE: GLM uses column major, be careful! C[j][i] = c_ij, C[j] = column j

  // General test R > R_0 + R_1 implies separating axis
  // one of them is an extent (size/2) and the other is a projection
  if (glm::abs(glm::dot(A[0], D)) >
      size1[0] +
      glm::dot(size2, glm::abs(glm::vec2(C[0][0], C[1][0]))))
    return false;
  if (glm::abs(glm::dot(A[1], D)) >
      size1[1] +
      glm::dot(size2, glm::abs(glm::vec2(C[0][1], C[1][1]))))
    return false;

  if (glm::abs(glm::dot(B[0], D)) >
      glm::dot(size1, glm::abs(C[0])) +
      size2[0])
    return false;
  if (glm::abs(glm::dot(B[1], D)) >
      glm::dot(size1, glm::abs(C[1])) +
      size2[1])
    return false;

  return true;
}

bool pointInPolygon(const glm::vec3 &point,
    const std::vector<glm::vec3> &polygon) {
  int n = polygon.size();
  bool result = false;
  for (int i = 0, j = n - 1; i < n; j = i++) {
    if ((polygon[i].y > point.y) != (polygon[j].y > point.y) &&
        point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) /
        (polygon[j].x - polygon[j].x) + polygon[i].x) result = !result;
  }
  return result;
}

float rayAABBIntersection(
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    const glm::vec3 &center,
    const glm::vec3 &size) {
  auto min = center - size/2.f;
  auto max = center + size/2.f;
  float tnear = -HUGE_VAL;
  float tfar = HUGE_VAL;
  for (int i = 0; i < 3; i++) {
    if (dir[i] == 0.f && (origin[i] < min[i] || origin[i] > max[i])) {
      return NO_INTERSECTION;
    }
    float t1 = (min[i] - origin[i]) / dir[i];
    float t2 = (max[i] - origin[i]) / dir[i];
    if (t1 > t2) std::swap(t1, t2);
    if (t1 > tnear) tnear = t1;
    if (t2 < tfar) tfar = t2;
    if (tnear > tfar) return NO_INTERSECTION;
    if (tfar < 0) return NO_INTERSECTION;
  }

  return tnear;
}

float rayBoxIntersection(
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    const Rect &box) {
  return NO_INTERSECTION;
}

float boxBoxCollision(
    const Rect &rect1,
    const glm::vec2 &v1,
    const Rect &rect2,
    const glm::vec2 &v2,
    float dt) {
  // Convert sizes to 'extents'
  const glm::vec2 size1 = rect1.size / 2.f;
  const glm::vec2 size2 = rect2.size / 2.f;
  // W = velocity relative to box 1
  const glm::vec2 W = v2 - v1;
  // D = C_1 - C_0 where C_0 is center of first box
  const glm::vec2 D0 = rect2.pos - rect1.pos;
  // Ending distance vector
  const glm::vec2 D1 = D0 + W * dt;
  // Columns of A/B correspond to the axis of the box
  const glm::mat2 A = glm::mat2(
      glm::vec2(glm::cos(rect1.angle), glm::sin(rect1.angle)),
      glm::vec2(-glm::sin(rect1.angle), glm::cos(rect1.angle)));
  const glm::mat2 B = glm::mat2(
      glm::vec2(glm::cos(rect2.angle), glm::sin(rect2.angle)),
      glm::vec2(-glm::sin(rect2.angle), glm::cos(rect2.angle)));
  // Each c_ij is A_i * B_j where A/B_k is an axis of that box
  const glm::mat2 C = glm::transpose(A) * B;
  // NOTE: GLM uses column major, be careful! C[j][i] = c_ij, C[j] = column j

  // Bit of a misnomer, the maximum, earliest intersection time
  float minT = -HUGE_VAL;
  // minimum last intersection time
  float maxT = HUGE_VAL;
  // collision if minT < maxT

#define check_axis(R0, R1, L) do { \
  /* Projection of velocity onto axis */ \
  float v = glm::dot(L, W); \
  float rstart = glm::dot(L, D0); \
  /* If there is no movement on this axis */ \
  if (glm::abs(v) < 0.001f) { \
    /* Just check for overlap at the beginning */ \
    if (glm::abs(rstart) > r0 + r1) { \
      return NO_INTERSECTION; \
    } else { \
    /* otherwise earliest and latest overlap at t=0 */ \
      minT = std::max(minT, 0.f); \
      maxT = std::min(maxT, dt); \
      break; \
    }\
  } \
  float rend = glm::dot(L, D1); \
  /* check for non overlap in the beginning and end. */ \
  if (glm::abs(rstart) > r0 + r1 && \
      glm::abs(rend) > r0 + r1) { \
    /* If the box remains on the same side, no global intersection */ \
    if (glm::sign(rstart) == glm::sign(rend)) { \
      return NO_INTERSECTION; \
    /* If the box is on a different side, find times of collision. */ \
    } else { \
      minT = std::max( \
          minT, \
          glm::abs((glm::abs(rstart) - (r0 + r1)) / v)); \
      maxT = std::min( \
          maxT, \
          glm::abs((glm::abs(rstart) + (r0 + r1)) / v)); \
      break; \
    } \
  } else { \
    /* deal with overlap at beginning or end. */ \
    minT = std::max( \
      minT, \
      std::max(0.f, (glm::abs(rstart) - (r0 + r1)) / glm::abs(v))); \
    maxT = std::min( \
      maxT, \
      std::min(dt, (glm::abs(rstart) + (r0 + r1)) / glm::abs(v))); \
  } \
} while (0);

  // General test R > R_0 + R_1 implies separating axis
  // one of them is an extent (size/2) and the other is a projection
  // Wl is the projection of W on the current axis L
  float r0, r1;
  glm::vec2 L;
  // For motion, also project the relative velocity vector W onto the axis
  r0 = size1[0];
  r1 = glm::dot(size2, glm::abs(glm::vec2(C[0][0], C[1][0])));
  L = A[0];
  check_axis(r0, r1, L);

  r0 = size1[1];
  r1 = glm::dot(size2, glm::abs(glm::vec2(C[0][1], C[1][1])));
  L = A[1];
  check_axis(r0, r1, L);

  r0 = glm::dot(size1, glm::abs(C[0]));
  r1 = size2[0];
  L = B[0];
  check_axis(r0, r1, L);

  r0 = glm::dot(size1, glm::abs(C[1]));
  r1 = size2[1];
  L = B[1];
  check_axis(r0, r1, L);

  // max(Earliest intersection time) is AFTER the min(Latest intersection time)
  if (minT > maxT || minT > dt) {
    return NO_INTERSECTION;
  } else {
    invariant(minT >= 0.f && minT <= dt, "bad intersection time");
    return minT;
  }
}
