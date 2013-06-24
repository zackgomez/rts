#ifndef SRC_RTS_EFFECTFACTORY_H_
#define SRC_RTS_EFFECTFACTORY_H_
#include <functional>
#include <v8.h>

namespace rts {

class ModelEntity;

typedef std::function<bool(float)> RenderFunction;

RenderFunction makeEffect(
    const std::string &name,
    v8::Handle<v8::Object> params);

RenderFunction makeEntityEffect(
    const ModelEntity *e,
    const std::string &name,
    v8::Handle<v8::Object> params);

}  // rts
#endif  // SRC_RTS_EFFECTFACTORY_H_