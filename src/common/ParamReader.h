#ifndef SRC_COMMON_PARAMREADER_H_
#define SRC_COMMON_PARAMREADER_H_
#include <string>
#include <vector>
#include <unordered_map>
#include <json/json.h>
#include <glm/glm.hpp>
#include "common/Exception.h"
#include "common/Logger.h"

// Global convenience methods
Json::Value getParam(const std::string &param);
bool hasParam(const std::string &param);

float fltParam(const std::string &param);
int intParam(const std::string &param);
glm::vec2 vec2Param(const std::string &param);
glm::vec3 vec3Param(const std::string &param);
glm::vec4 vec4Param(const std::string &param);
std::string strParam(const std::string &param);
std::vector<std::string> arrParam(const std::string &param);
template<class T>
void setParam(const std::string &param, const T& value);


class param_exception : public exception_with_trace {
 public:
  explicit param_exception(const std::string &msg) :
    exception_with_trace(msg) {
    }
};


class ParamReader {
 public:
  static ParamReader *get();

  void loadFile(const char *filename);

  bool hasParam(const std::string &param) const;
  Json::Value getParam(const std::string &param) const;
  template<class T>
  void setParam(const std::string &param, const T& value) {
    params_[param] = toJson(value);
  }

  // Returns a checksum of the root file loaded.  Any included files have
  // their inclusion noted, but not the contents.
  uint32_t getFileChecksum() const {
    return fileChecksum_;
  }

 private:
  ParamReader();
  void flattenValue(const Json::Value &v, const std::string &prefix = "");

  std::unordered_map<std::string, Json::Value> params_;
  uint32_t fileChecksum_;
};

template<class T>
void setParam(const std::string &param, const T& value) {
  ParamReader::get()->setParam(param, value);
}

#endif  // SRC_COMMON_PARAMREADER_H_
