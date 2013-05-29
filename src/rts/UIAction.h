#pragma once
#include <json/json.h>
#include "common/Types.h"

namespace rts {

class UIAction {
 public:
  explicit UIAction(id_t owner, const Json::Value &def)
  : owner_(owner), def_(def)
  {}

  id_t getOwner() const {
    return owner_;
  }
  const Json::Value &getRawDefinition() const {
    return def_;
  }

  std::string getTooltip() const;
  enum class TargetingType {
    NONE,
    LOCATION,
  };
  TargetingType getTargeting() const;

  std::string getName() const;
  std::string getIconTextureName() const;

 private:
  id_t owner_;
  Json::Value def_;
};

};