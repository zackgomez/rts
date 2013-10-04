#ifndef SRC_RTS_EFFECTFACTORY_H_
#define SRC_RTS_EFFECTFACTORY_H_
#include <functional>
#include <string>
#include <v8.h>
#include "rts/EffectManager.h"

namespace rts {

void add_jseffect(const std::string &name, v8::Handle<v8::Object> params);

class ModelEntity;

typedef std::function<bool(float)> RenderFunction;

}  // rts
#endif  // SRC_RTS_EFFECTFACTORY_H_
