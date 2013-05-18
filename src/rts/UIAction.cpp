#include "rts/UIAction.h"
#include <json/json.h>
#include <sstream>
#include "common/util.h"

namespace rts {

std::string UIAction::getTooltip() const {
  std::stringstream ss;

  std::string type = def_["type"].asString();
  if (type == "production") {
    ss << def_["prod_name"].asString() << '\n'
      << "Req: " << def_["req_cost"].asFloat() << '\n'
      << "Time: " << def_["time_cost"].asFloat();
  } else {
    invariant_violation("unknown action type " + type);
  }

  return ss.str();
}

UIAction::TargetingType UIAction::getTargeting() const {
  return UIAction::TargetingType::NONE;
}

std::string UIAction::getName() const {
  invariant(def_.isMember("name"), "missing action name");
  return def_["name"].asString();
}

std::string UIAction::getIconTextureName() const {
  invariant(def_.isMember("texture"), "missing action texture");
  return def_["texture"].asString();
}

};  // rts
