#include <boost/algorithm/string.hpp>
#include <boost/crc.hpp>
#include "ParamReader.h"
#include "util.h"

ParamReader::ParamReader() {
  logger_ = Logger::getLogger("ParamReader");
}

ParamReader * ParamReader::get() {
  static ParamReader pr;
  return &pr;
}

void ParamReader::loadFile(const char *filename) {
  std::ifstream file(filename);
  Json::Reader reader;
  if (!reader.parse(file, root_)) {
    logger_->fatal() << "Cannot load param file " << filename << "\n"
                     << reader.getFormattedErrorMessages();
    return;
  }

  // Checksum file
  const size_t bufsz = 1024;
  char buf[bufsz];
  boost::crc_32_type checksum;
  file.seekg(std::ios::beg);
  while (file.read(buf, bufsz)) {
    checksum.process_bytes(buf, file.gcount());
  }
  fileChecksum_ = checksum.checksum();
  LOG(INFO) << "Params file '" << filename << "' checksum: "
    << std::hex << fileChecksum_ << '\n' << std::dec;

  // Read in includes from header comment
  // format is '// @include param file'
  std::stringstream commentstream(root_.getComment(Json::commentBefore));
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
      if (root_.isMember(paramname)) {
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

      root_[paramname] = param;

      LOG(INFO) << "Included file " << filename << " as param " <<
        paramname << '\n';
    } else {
      LOG(WARNING) << "unknown include comment: " << comment << '\n';
    }
  }
}

void ParamReader::printParams() const {
  logger_->info() << root_.toStyledString() << "\n";
}

bool ParamReader::hasFloat(const std::string &param) const {
  Json::Value val = getParamHelper(param);
  return !val.isNull() && val.isNumeric();
}

bool ParamReader::hasString(const std::string &param) const {
  Json::Value val = getParamHelper(param);
  return !val.isNull() && val.isString();
}

Json::Value ParamReader::getParamHelper(const std::string &param) const {
  Json::Value val = root_;
  std::vector<std::string> param_split;
  boost::split(param_split, param, boost::is_any_of("."));
  for (auto& child : param_split) {
    val = val.get(child, Json::Value());
    if (val.isNull()) {
      break;
    }
  }
  return val;
}

Json::Value ParamReader::getParam(const std::string &param) const {
  Json::Value val = getParamHelper(param);
  invariant(!val.isNull(), "param not found: " + param);
  return val;
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

Json::Value getParam(const std::string &param) {
  Json::Value val = ParamReader::get()->getParam(param);
  return val;
}
