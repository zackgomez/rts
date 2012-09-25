#include "common/Collision.h"
#include <algorithm>  // for min/max
#include "common/Logger.h"
#include "common/util.h"

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

  float t, v;
  // Bit of a misnomer, the maximum, earliest intersection time
  float minT = -HUGE_VAL;
  // minimum last intersection time
  float maxT = HUGE_VAL;
  // collision if minT < maxT

  // General test R > R_0 + R_1 implies separating axis
  // one of them is an extent (size/2) and the other is a projection
  // Wl is the projection of W on the current axis L
  float rstart, rend, r0, r1;
  // For motion, also project the relative velocity vector W onto the axis
  r0 = size1[0];
  r1 = glm::dot(size2, glm::abs(glm::vec2(C[0][0], C[1][0])));
  rstart = glm::dot(A[0], D0);
  rend = glm::dot(A[0], D1);
  // If separated at both endpoints, and moving same direction relative to
  // other box, no collision
  if (glm::abs(rstart) > r0 + r1 &&
      glm::abs(rend) > r0 + r1 &&
      glm::sign(rstart) == glm::sign(rend)) {
    return NO_INTERSECTION;
  } else {
    // Projection of velocity onto axis
    v = glm::dot(A[0], W);
    // always overlapping
    if (v != 0) {
      // Otherwise calculate collision time as (starting_dist / speed)
      // could be negative, if there is a collision at time 0, just take
      // intersection time as 0 in that case
      t = (glm::abs(rstart) - r0 - r1) / v;
      // Then the minT is the earliest intersection
      minT = std::max(minT, std::max(t, 0.f));
      t = (r0 + r1 - glm::abs(rend)) / v;
      maxT = std::min(maxT, t);
    }
  }

  r0 = size1[1];
  r1 = glm::dot(size2, glm::abs(glm::vec2(C[0][1], C[1][1])));
  rstart = glm::dot(A[1], D0);
  rend = glm::dot(A[1], D1);
  if (glm::abs(rstart) > r0 + r1 &&
      glm::abs(rend) > r0 + r1 &&
      glm::sign(rstart) == glm::sign(rend)) {
    return NO_INTERSECTION;
  } else {
    v = glm::dot(A[1], W);
    if (v != 0) {
      t = (r0 + r1 - glm::abs(rstart)) / v;
      minT = std::max(minT, std::max(t, 0.f));
      t = (r0 + r1 - glm::abs(rend)) / v;
      maxT = std::min(maxT, t);
    }
  }

  r0 = glm::dot(size1, glm::abs(C[0]));
  r1 = size2[0];
  rstart = glm::dot(B[0], D0);
  rend = glm::dot(B[0], D1);
  if (glm::abs(rstart) > r0 + r1 &&
      glm::abs(rend) > r0 + r1 &&
      glm::sign(rstart) == glm::sign(rend)) {
    return NO_INTERSECTION;
  } else {
    v = glm::dot(B[0], W);
    if (v != 0) {
      t = (r0 + r1 - glm::abs(rstart)) / v;
      minT = std::max(minT, std::max(t, 0.f));
      t = (r0 + r1 - glm::abs(rend)) / v;
      maxT = std::min(maxT, t);
    }
  }

  r0 = glm::dot(size1, glm::abs(C[1]));
  r1 = size2[1];
  rstart = glm::dot(B[1], D0);
  rend = glm::dot(B[1], D1);
  if (glm::abs(rstart) > r0 + r1 &&
      glm::abs(rend) > r0 + r1 &&
      glm::sign(rstart) == glm::sign(rend)) {
    return NO_INTERSECTION;
  } else {
    v = glm::dot(B[1], W);
    if (v != 0) {
      t = (r0 + r1 - glm::abs(rstart)) / v;
      minT = std::max(minT, std::max(t, 0.f));
      t = (r0 + r1 - glm::abs(rend)) / v;
      maxT = std::min(maxT, t);
    }
  }

  if (minT > maxT) {
    return false;
  }
  invariant(minT >= 0.f && minT <= dt, "bad intersection time");
  return minT;
}
