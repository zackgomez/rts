#pragma once
#include <json/json.h>

namespace rts {

struct UIAction {
  UIAction() {}
  UIAction(const Json::Value &action, int idx)
  : actor_action(action), action_idx(idx)
  {}

  std::string getTooltip() const;

  Json::Value actor_action;
  int action_idx;
};

};
