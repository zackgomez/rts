#include "glm.h"
#include <cstdlib>

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
  return glm::vec3(v[0].asFloat(), v[1].asFloat(), v[2].asFloat());
}

glm::vec4 toVec4(const Json::Value &v) {
  return glm::vec4(v[0].asFloat(), v[1].asFloat(), v[2].asFloat(), v[3].asFloat());
}

rts::id_t toID(const Json::Value &v) {
  return v.asUInt64();
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
