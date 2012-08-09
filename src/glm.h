#pragma once
#include <cstdint>
#include <cmath>
#include <set>
#include <ostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <json/json.h>
#include "Types.h"

std::ostream& operator<< (std::ostream &os, const glm::vec2 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec3 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec4 &v);

Json::Value toJson(const glm::vec2 &v);
Json::Value toJson(const glm::vec3 &v);
Json::Value toJson(const glm::vec4 &v);
Json::Value toJson(uint64_t n);
Json::Value toJson(int64_t n);
rts::id_t toID(const Json::Value &v);

template <class T>
Json::Value toJson(const std::set<T> &entities) {
  Json::Value v;
for (const T& t : entities) {
    v.append(toJson(t));
  }

  return v;
}

glm::vec2 toVec2(const Json::Value &v);
glm::vec3 toVec3(const Json::Value &v);
glm::vec4 toVec4(const Json::Value &v);

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt);

inline float deg2rad(float deg) {
  return M_PI / 180.f * deg;
}

inline float rad2deg(float rad) {
  return 180.f / M_PI * rad;
}

void seedRandom(unsigned int seed);
// Returns float in [0,1]
float frand();

float addAngles(float a, float b);
