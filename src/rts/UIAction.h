#pragma once
#include <json/json.h>
#include "common/Types.h"

namespace rts {

struct UIAction {
 public:
  enum class TargetingType {
    NONE,
    LOCATION,
  };

  id_t owner;
  std::string name;
  std::string icon;
  std::string tooltip;
  TargetingType targeting;
};

};