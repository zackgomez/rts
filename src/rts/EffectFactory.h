#ifndef SRC_RTS_EFFECTFACTORY_H_
#define SRC_RTS_EFFECTFACTORY_H_
#include <functional>
#include <string>
#include <json/json.h>
#include "rts/EffectManager.h"

namespace rts {

void add_effect(const std::string &name, const Json::Value& params);

}  // rts
#endif  // SRC_RTS_EFFECTFACTORY_H_
