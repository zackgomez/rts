#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ostream>
#include <json/json.h>

std::ostream& operator<< (std::ostream &os, const glm::vec2 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec3 &v);
std::ostream& operator<< (std::ostream &os, const glm::vec4 &v);

Json::Value toJson(const glm::vec2 &v);
Json::Value toJson(const glm::vec3 &v);
Json::Value toJson(const glm::vec4 &v);

glm::vec2 toVec2(const Json::Value &v);
glm::vec3 toVec3(const Json::Value &v);
glm::vec4 toVec4(const Json::Value &v);

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt);
