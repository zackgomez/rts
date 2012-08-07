#pragma once
#include <map>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "Logger.h"
#include "glm.h"

// Global convenience methods
const float fltParam(const std::string &param);
const float intParam(const std::string &param);
const glm::vec2 vec2Param(const std::string &param);
const glm::vec3 vec3Param(const std::string &param);
const std::string strParam(const std::string &param);
const std::vector<std::string> arrParam(const std::string &param);

void setParam(const std::string &key, float value);
void setParam(const std::string &key, const std::string &value);


class ParamReader
{
public:
  static ParamReader *get();

  void loadFile(const char *filename);

  bool hasFloat(const std::string &param) const;
  bool hasString(const std::string &param) const;
  Json::Value getParam(const std::string &param) const;

  void printParams() const;

private:
  ParamReader();
  Json::Value getParamHelper(const std::string &param) const;
  LoggerPtr logger_;

  Json::Value root_;
};

