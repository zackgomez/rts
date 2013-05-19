#ifndef SRC_RTS_GAMESCRIPT_H_
#define SRC_RTS_GAMESCRIPT_H_
#include <v8.h>

namespace rts {
class GameScript {
public:
  ~GameScript();
  void init();

private:
  v8::Persistent<v8::Context> context_;
  v8::Isolate *isolate_ = nullptr;
};

};

#endif  // SRC_RTS_GAMESCRIPT_H_
