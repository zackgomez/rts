#include "ParamReader.h"
#include <cassert>
#include <boost/algorithm/string.hpp>
#include "glm.h"

ParamReader::ParamReader()
{
  logger_ = Logger::getLogger("ParamReader");
}

ParamReader * ParamReader::get()
{
  static ParamReader pr;
  return &pr;
}

void ParamReader::loadFile(const char *filename)
{
  std::ifstream file(filename);
  std::string data((std::istreambuf_iterator<char>(file)), 
      std::istreambuf_iterator<char>());
  Json::Reader reader;
  if (!reader.parse(data, root_))
  {
    logger_->fatal() << "Cannot load param file " << filename << "\n"
      << reader.getFormattedErrorMessages();
    return;
  }
}

void ParamReader::printParams() const
{
  logger_->info() << root_.toStyledString() << "\n";
}

bool ParamReader::hasFloat(const std::string &param) const
{
  Json::Value val = getParamHelper(param);
  return !val.isNull() && val.isNumeric();
}

bool ParamReader::hasString(const std::string &param) const
{
  Json::Value val = getParamHelper(param);
  return !val.isNull() && val.isString();
}

Json::Value ParamReader::getParamHelper(const std::string &param) const
{
  Json::Value val = root_;
  std::vector<std::string> param_split;
  boost::split(param_split, param, boost::is_any_of("."));
  for (auto& child : param_split)  
  {
    val = val.get(child, Json::Value());
    if (val.isNull())
    {
      break;
    }
  }
  return val;
}

Json::Value ParamReader::getParam(const std::string &param) const
{
  Json::Value val = getParamHelper(param);
  if (val.isNull())
  {
    logger_->fatal() << "Cannot find param " << param << "\n";
    assert(false);
  }
  return val;
}

const std::string strParam(const std::string &param)
{
  Json::Value val = ParamReader::get()->getParam(param);
  assert(val.isString());
  return val.asString();
}

const float fltParam(const std::string &param)
{
  Json::Value val = ParamReader::get()->getParam(param);
  assert(val.isNumeric());
  return val.asFloat();
}

const float intParam(const std::string &param)
{
  Json::Value val = ParamReader::get()->getParam(param);
  assert(val.isInt());
  return val.asInt();
}

const std::vector<std::string> arrParam(const std::string &param)
{
  Json::Value arr = ParamReader::get()->getParam(param);
  std::vector<std::string> result;
  for (unsigned int i = 0; i < arr.size(); i++)
  {
    assert(arr[i].isString());
    result.push_back(arr[i].asString());
  }
  return result;
}

const glm::vec2 vec2Param(const std::string &param)
{
  return toVec2(ParamReader::get()->getParam(param));
}
