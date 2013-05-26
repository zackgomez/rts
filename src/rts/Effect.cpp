#include "rts/Effect.h"
#include <v8.h>
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Game.h"
#include "rts/GameScript.h"
#include "rts/MessageHub.h"
#include "rts/Renderer.h"

namespace rts {

Effect createAuraEffect(const Json::Value &effect) {
  invariant(effect.isMember("aura_type"), "missing aura type");
  invariant(effect.isMember("radius"), "missing aura radius");
  // TODO(zacharyg): remove generic amount
  invariant(effect.isMember("amount"), "missing aura amount");

  const std::string type = effect["aura_type"].asString();
  const float radius = effect["radius"].asFloat();
  const float amount = effect["amount"].asFloat();

  return [=](const Actor *a, float dt) -> bool {
    if (type == "healing") {
        Renderer::get()->getNearbyEntities(
          a->getPosition(),
          radius,
          [&] (const GameEntity *e) -> bool {
            if (e->getID() != a->getID() && e->getPlayerID() == a->getPlayerID()) {
              MessageHub::get()->sendHealMessage(
                a->getID(),
                e->getID(),
                dt * amount);
            }
            return true;
          });
    } else {
      invariant_violation("unknown aura type " + type);
    }
    return true;
  };
}

Effect createJSEffect(const Json::Value &effect) {
  using namespace v8;
  invariant(effect.isMember("function"), "missing function name");
  std::string func_name = effect["function"].asString();

  return [=](const Actor *a, float dt) -> bool {
    auto script = Game::get()->getScript();
    HandleScope handle_scope(script->getIsolate());

    Handle<Object> global = Game::get()->getScript()->getGlobal();
    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(a->getID()), Number::New(dt)};

    TryCatch try_catch;
    auto fn = Handle<Function>::Cast(global->Get(String::New(func_name.c_str())));
    Handle<Value> result = fn->Call(global, argc, argv);
    if (result.IsEmpty()) {
      LOG(ERROR) << "Error running effect: "
        << *String::AsciiValue(try_catch.Exception());
      return false;
    }

    return result->BooleanValue();
  };
}

Effect createEffect(const std::string &name) {
  auto effect = getParam("effects." + name);

  invariant(effect.isMember("type"), "missing effects type");
  const std::string type = effect["type"].asString();

  if (type == "aura") {
    return createAuraEffect(effect);
  } else if (type == "script") {
    return createJSEffect(effect);
  } else {
    invariant_violation("unknown effect type " + type);
  }
}

};  // rts

