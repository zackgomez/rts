#ifndef SRC_RTS_EFFECT_H_
#define SRC_RTS_EFFECT_H_
#include <functional>
#include <string>

namespace rts {
class Actor;

typedef std::function<bool(const Actor *, float dt)> Effect;

Effect createEffect(const std::string &name);
};  // rts
#endif  // SRC_RTS_EFFECT_H_
