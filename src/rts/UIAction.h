#pragma once
#include <json/json.h>

namespace rts {

struct UIAction {
  Json::Value actor_action;
  int action_idx;
};

};
