#include "glm.h"


std::ostream& operator<< (std::ostream &os, const glm::vec2 &v)
{
    os << v.x << ' ' << v.y;
    return os;
}

std::ostream& operator<< (std::ostream &os, const glm::vec3 &v)
{
    os << v.x << ' ' << v.y << ' ' << v.z;
    return os;
}

std::ostream& operator<< (std::ostream &os, const glm::vec4 &v)
{
    os << v.x << ' ' << v.y << ' ' << v.z << ' ' << v.w;
    return os;
}

Json::Value toJson(const glm::vec2 &v)
{
    Json::Value jv;
    jv[0] = v[0]; jv[1] = v[1];

    return jv;
}

Json::Value toJson(const glm::vec3 &v)
{
    Json::Value jv;
    jv[0] = v[0]; jv[1] = v[1]; jv[2] = v[2];

    return jv;
}

Json::Value toJson(const glm::vec4 &v)
{
    Json::Value jv;
    jv[0] = v[0]; jv[1] = v[1]; jv[2] = v[2]; jv[3] = v[3];

    return jv;
}

glm::vec2 toVec2(const Json::Value &v)
{
    return glm::vec2(v[0].asFloat(), v[1].asFloat());
}

glm::vec3 toVec3(const Json::Value &v)
{
    return glm::vec3(v[0].asFloat(), v[1].asFloat(), v[2].asFloat());
}

glm::vec4 toVec4(const Json::Value &v)
{
    return glm::vec4(v[0].asFloat(), v[1].asFloat(), v[2].asFloat(), v[3].asFloat());
}

glm::vec3 applyMatrix(const glm::mat4 &mat, const glm::vec3 &pt)
{
    glm::vec4 p = mat * glm::vec4(pt, 1.f);
    p /= p.w;
    return glm::vec3(p);
}
