#include "common/util.h"
#include <cstdlib>

void __invariant(bool condition, const std::string &message) {
  if (!condition) {
    throw violation_exception(message);
  }
}

std::ostream& operator<< (std::ostream &os, const glm::vec2 &v) {
  os << v.x << ' ' << v.y;
  return os;
}

std::ostream& operator<< (std::ostream &os, const glm::vec3 &v) {
  os << v.x << ' ' << v.y << ' ' << v.z;
  return os;
}

std::ostream& operator<< (std::ostream &os, const glm::vec4 &v) {
  os << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.w;
  return os;
}

Json::Value toJson(const glm::vec2 &v) {
  Json::Value jv;
  jv[0] = v[0];
  jv[1] = v[1];

  return jv;
}

Json::Value toJson(const glm::vec3 &v) {
  Json::Value jv;
  jv[0] = v[0];
  jv[1] = v[1];
  jv[2] = v[2];

  return jv;
}

Json::Value toJson(const glm::vec4 &v) {
  Json::Value jv;
  jv[0] = v[0];
  jv[1] = v[1];
  jv[2] = v[2];
  jv[3] = v[3];

  return jv;
}

Json::Value toJson(uint64_t n) {
  Json::Value v;
  v = (Json::Value::UInt64) n;
  return v;
}

Json::Value toJson(int64_t n) {
  Json::Value v;
  v = (Json::Value::Int64) n;
  return v;
}

glm::vec2 toVec2(const Json::Value &v) {
  return glm::vec2(v[0].asFloat(), v[1].asFloat());
}

glm::vec3 toVec3(const Json::Value &v) {
  return
    glm::vec3(
      v[0].asFloat(),
      v[1].asFloat(),
      v[2].asFloat());
}

glm::vec4 toVec4(const Json::Value &v) {
  return
    glm::vec4(
      v[0].asFloat(),
      v[1].asFloat(),
      v[2].asFloat(),
      v[3].asFloat());
}

rts::id_t toID(const Json::Value &v) {
  return v.asUInt64();
}

rts::tick_t toTick(const Json::Value &v) {
  return v.asInt64();
}

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt, bool homo) {
  glm::vec4 p = mat * glm::vec4(pt, homo ? 1.f : 0.f);
  if (homo) {
    p /= p.w;
  }
  return glm::vec3(p);
}

void seedRandom(unsigned int seed) {
  srand(seed);
}

float frand() {
  return rand() / static_cast<float>(RAND_MAX);
}

float addAngles(float a, float b) {
  float ret = a + b;
  while (ret > 180.f) {
    ret -= 360.f;
  }
  while (ret < -180.f) {
    ret += 360.f;
  }

  return ret;
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

bool boxInBox(
    const glm::vec2 &pos1,
    const glm::vec2 &size1,
    float angle1,
    const glm::vec2 &pos2,
    const glm::vec2 &size2,
    float angle2) {
  return boxInBoxHelper(pos1, size1, angle1, pos2, size2, angle2) &&
         boxInBoxHelper(pos2, size2, angle2, pos1, size1, angle1);
}
