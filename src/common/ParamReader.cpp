#include "common/ParamReader.h"
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "common/Checksum.h"
#include "common/Clock.h"
#include "common/util.h"

ParamReader::ParamReader() {
}

ParamReader * ParamReader::get() {
  static ParamReader instance;
  return &instance;
}

void ParamReader::loadFile(const char *filename) {
  std::ifstream file(filename);
  if (!file) {
    throw file_exception(std::string("Unable to read file ") + filename);
  }
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(file, root)) {
    throw file_exception(std::string("Cannot parse param file ") + filename
        + " : " + reader.getFormattedErrorMessages());
  }

  // Checksum entire file
  file.seekg(std::ios::beg);
  fileChecksum_ = Checksum().process(file).getChecksum();
  LOG(INFO) << "Params file '" << filename << "' checksum: "
    << Checksum::checksumToString(fileChecksum_) << '\n';

  // Read in includes from header comment
  // format is '// @include param file'
  std::stringstream commentstream(root.getComment(Json::commentBefore));
  std::string comment;
  while (std::getline(commentstream, comment)) {
    if (comment.empty()) {
      continue;
    }
    std::vector<std::string> commentsplit;
    boost::split(commentsplit, comment, boost::is_any_of(" "));
    // Check format
    if (commentsplit.size() == 4 && commentsplit[1] == "@include") {
      const std::string &filename = commentsplit[3];
      const std::string &paramname = commentsplit[2];

      // Param must not already exist
      if (root.isMember(paramname)) {
        throw exception_with_trace(
          "asked to include param to already existing name");
      }
      // Open file
      std::ifstream includedfile(filename.c_str());
      if (!includedfile) {
        throw exception_with_trace("Unable to open file " + filename);
      }
      // Parse
      Json::Value param;
      if (!reader.parse(includedfile, param)) {
        throw exception_with_trace("Unable to read param file " + filename +
            ": " + reader.getFormattedErrorMessages());
      }

      root[paramname] = param;

      LOG(INFO) << "Included file " << filename << " as param " <<
        paramname << '\n';
    } else {
      LOG(WARNING) << "unknown include comment: " << comment << '\n';
    }
  }

  flattenValue(root);
}

void ParamReader::flattenValue(
    const Json::Value &root,
    const std::string &prefix) {
  for (auto it = root.begin(); it != root.end(); it++) {
    std::string name = prefix;
    if (!name.empty()) {
      name += '.';
    }
    name += it.memberName();

    if ((*it).isObject()) {
      flattenValue(*it, name);
    }
    // always save param, so people can access object params too
    params_[name] = *it;
  }
}

bool ParamReader::hasParam(const std::string &param) const {
  return params_.find(param) != params_.end();
}

Json::Value ParamReader::getParam(const std::string &param) const {
  auto it = params_.find(param);
  if (it == params_.end()) {
    LOG(FATAL) << "missing param: " << param << '\n';
    throw param_exception("Param " + param + " not found.\n");
  }

  return it->second;
}

// Global helpers

Json::Value getParam(const std::string &param) {
  record_section("getParam");
  Json::Value val = ParamReader::get()->getParam(param);
  return val;
}

bool hasParam(const std::string &param) {
  return ParamReader::get()->hasParam(param);
}

std::string strParam(const std::string &param) {
  Json::Value val = ParamReader::get()->getParam(param);
  invariant(val.isString(), "unknown or badly typed param");
  return val.asString();
}

float fltParam(const std::string &param) {
  Json::Value val = ParamReader::get()->getParam(param);
  invariant(val.isNumeric(), "unknown or badly typed param");
  return val.asFloat();
}

int intParam(const std::string &param) {
  Json::Value val = ParamReader::get()->getParam(param);
  invariant(val.isInt(), "unknown or badly typed param");
  return val.asInt();
}

std::vector<std::string> arrParam(const std::string &param) {
  Json::Value arr = ParamReader::get()->getParam(param);
  std::vector<std::string> result;
  for (unsigned int i = 0; i < arr.size(); i++) {
    invariant(arr[i].isString(), "unknown or badly typed param");
    result.push_back(arr[i].asString());
  }
  return result;
}

glm::vec2 vec2Param(const std::string &param) {
  Json::Value arr = ParamReader::get()->getParam(param);
  invariant(arr.isArray() && arr.size() == 2,
      "vec2 param not found or not correctly sized array");
  return toVec2(arr);
}

glm::vec3 vec3Param(const std::string &param) {
  Json::Value arr = ParamReader::get()->getParam(param);
  invariant(arr.isArray() && arr.size() == 3,
      "vec3 param not found or not correctly sized array");
  return toVec3(arr);
}

glm::vec4 vec4Param(const std::string &param) {
  Json::Value arr = ParamReader::get()->getParam(param);
  invariant(arr.isArray() && arr.size() == 4,
      "vec4 param not found or not correctly sized array");
  return toVec4(arr);
}
