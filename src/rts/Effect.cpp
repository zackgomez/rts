#include "rts/Effect.h"
#include <v8.h>
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Game.h"
#include "rts/GameScript.h"
#include "rts/MessageHub.h"
#include "rts/Renderer.h"

namespace rts {

Effect createEffect(const std::string &name) {
  using namespace v8;

  return [=](const Actor *a, float dt) -> bool {
    auto script = Game::get()->getScript();
    HandleScope handle_scope(script->getIsolate());

    Handle<Object> global = Game::get()->getScript()->getGlobal();
    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(a->getID()), Number::New(dt)};

    TryCatch try_catch;
    auto fn = Handle<Function>::Cast(global->Get(String::New(name.c_str())));
    Handle<Value> result = fn->Call(global, argc, argv);
    if (result.IsEmpty()) {
      LOG(ERROR) << "Error running effect " << name << ": "
        << *String::AsciiValue(try_catch.Exception()) << '\n';
      return false;
    }

    return result->BooleanValue();
  };
}

};  // rts
