#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>
#include <cmath>

#include <ostream>
#include <json/json.h>

std::ostream& operator<< (std::ostream &os, const glm::vec2 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec3 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec4 &v);

Json::Value toJson(const glm::vec2 &v);
Json::Value toJson(const glm::vec3 &v);
Json::Value toJson(const glm::vec4 &v);
Json::Value toJson(uint64_t n);
Json::Value toJson(int64_t n);

glm::vec2 toVec2(const Json::Value &v);
glm::vec3 toVec3(const Json::Value &v);
glm::vec4 toVec4(const Json::Value &v);

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt);

inline float deg2rad(float deg)
{
    return M_PI / 180.f * deg;
}

inline float rad2deg(float rad)
{
    return 180.f / M_PI * rad;
}

void seedRandom(unsigned int seed);
// Returns float in [0,1]
float frand();

float addAngles(float a, float b);
