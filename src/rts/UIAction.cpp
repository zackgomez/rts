#include "rts/UIAction.h"
#include <json/json.h>
#include <sstream>

namespace rts {

std::string UIAction::getTooltip() const {
  std::stringstream ss;

  if (actor_action["type"].asString() == "production") {
    ss << actor_action["prod_name"].asString() << '\n'
      << "Req: " << actor_action["req_cost"].asFloat() << '\n'
      << "Time: " << actor_action["time_cost"].asFloat();
  }

  return ss.str();
}

};  // rts
