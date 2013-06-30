#pragma once
#include <json/json.h>
#include "common/Types.h"

namespace rts {

struct UIAction {
 public:
  // Keep in sync with the javascript constants
  enum class TargetingType {
    NONE = 0,
    LOCATION = 1,
    ENEMY = 2,
    ALLY = 3,
  };
  enum ActionState {
    DISABLED = 0,
    ENABLED = 1,
    COOLDOWN = 2,
  };

  id_t owner;
  std::string name;
  char hotkey;
  std::string icon;
  std::string tooltip;
  TargetingType targeting;
  float range;
  ActionState state;
  // Only relevant in COOLDOWN state
  float cooldown;
};
};
