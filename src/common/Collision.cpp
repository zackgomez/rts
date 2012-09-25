#include "common/Collision.h"

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
    const glm::vec2 &pos1,
    glm::vec2 size1,
    float angle1,
    const glm::vec2 &pos2,
    glm::vec2 size2,
    float angle2) {
  // Convert sizes to 'extents'
  size1 /= 2.f;
  size2 /= 2.f;
  // D = C_1 - C_0 where C_0 is center of first box
  const glm::vec2 D = pos2 - pos1;
  // Columns of A/B correspond to the axis of the box
  const glm::mat2 A = glm::mat2(
      glm::vec2(glm::cos(angle1), glm::sin(angle1)),
      glm::vec2(-glm::sin(angle1), glm::cos(angle1)));
  const glm::mat2 B = glm::mat2(
      glm::vec2(glm::cos(angle2), glm::sin(angle2)),
      glm::vec2(-glm::sin(angle2), glm::cos(angle2)));
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

// Helper function for boxInBox; checks if the separating axis is an axis
// of the first input box.
bool boxInBoxHelper(
    const glm::vec2 &pos1,
    const glm::vec2 &size1,
    float angle1,
    const glm::vec2 &pos2,
    const glm::vec2 &size2,
    float angle2) {
  glm::vec2 minCoord = glm::vec2(HUGE_VAL);
  glm::vec2 maxCoord = -glm::vec2(HUGE_VAL);
  glm::vec2 boxCoords[4];
  // The four corners of box2
  boxCoords[0] = glm::vec2(size2.x / 2, size2.y / 2);
  boxCoords[1] = glm::vec2(-size2.x / 2, size2.y / 2);
  boxCoords[2] = glm::vec2(-size2.x / 2, -size2.y / 2);
  boxCoords[3] = glm::vec2(size2.x / 2, -size2.y / 2);
  // get the second box relative to the first
  for (int i = 0; i < 4; i++) {
    boxCoords[i] = glm::vec2(
        glm::dot(glm::vec2(glm::cos(angle2), -glm::sin(angle2)), boxCoords[i]),
        glm::dot(glm::vec2(glm::sin(angle2), glm::cos(angle2)), boxCoords[i]));
    boxCoords[i] += pos2 - pos1;
    boxCoords[i] = glm::vec2(
        glm::dot(glm::vec2(glm::cos(angle1), glm::sin(angle1)), boxCoords[i]),
        glm::dot(glm::vec2(-glm::sin(angle1), glm::cos(angle1)), boxCoords[i]));
    if (boxCoords[i].x > maxCoord.x) maxCoord.x = boxCoords[i].x;
    if (boxCoords[i].x < minCoord.x) minCoord.x = boxCoords[i].x;
    if (boxCoords[i].y > maxCoord.y) maxCoord.y = boxCoords[i].y;
    if (boxCoords[i].y < minCoord.y) minCoord.y = boxCoords[i].y;
  }
  if (minCoord.x >= size1.x / 2 || maxCoord.x < -size1.x / 2 ||
      minCoord.y >= size1.y / 2 || maxCoord.y < -size1.y / 2) {
    return false;
  }
  return true;
}

bool boxInBoxOld(
    const glm::vec2 &pos1,
    const glm::vec2 &size1,
    float angle1,
    const glm::vec2 &pos2,
    const glm::vec2 &size2,
    float angle2) {
  return boxInBoxHelper(pos1, size1, angle1, pos2, size2, angle2) &&
         boxInBoxHelper(pos2, size2, angle2, pos1, size1, angle1);
}
