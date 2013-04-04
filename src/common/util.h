#ifndef SRC_COMMON_UTIL_H_
#define SRC_COMMON_UTIL_H_

#include <string>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <set>
#include <ostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <json/json.h>
#include "common/Exception.h"
#include "common/Types.h"

typedef std::chrono::high_resolution_clock hi_res_clock;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;

/*
 * A variation of assert that throws an exception instead of calling abort.
 * The exception message includes the passed in message.
 * @param condition If false, will throw ViolationException
 * @param message A message describing the failure, when it occurs
 */
#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define invariant(condition, message) __invariant(condition, \
    std::string() + __FILE__ ":" S__LINE__ " " #condition " - " + message)
[[noreturn]] void invariant_violation(const std::string &message);

void __invariant(bool condition, const std::string &message);

std::ostream& operator<< (std::ostream &os, const glm::vec2 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec3 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec4 &v);

Json::Value toJson(const glm::vec2 &v);
Json::Value toJson(const glm::vec3 &v);
Json::Value toJson(const glm::vec4 &v);
Json::Value toJson(uint64_t n);
Json::Value toJson(int64_t n);
rts::id_t   toID(const Json::Value &v);
rts::tick_t toTick(const Json::Value &v);

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

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt,
    bool homo = true);

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

#endif  // SRC_COMMON_UTIL_H_
