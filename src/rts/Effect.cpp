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

Effect createJSTestEffect(const Json::Value &effect) {
  using namespace v8;
  return [=](const Actor *a, float dt) -> bool {
    auto script = Game::get()->getScript();
    HandleScope handle_scope(script->getIsolate());

    Handle<Object> global = Game::get()->getScript()->getGlobal();
    const int argc = 2;
    Handle<Value> argv[argc] = {script->getEntity(a->getID()), Number::New(dt)};
    auto fn = Handle<Function>::Cast(global->Get(String::New("testEffect")));

    Handle<Value> result = fn->Call(global, argc, argv);
    return result->BooleanValue();
  };
}

Effect createResourceEffect(const Json::Value &effect) {
  invariant(effect.isMember("amount"), "missing resource rate");
  invariant(effect.isMember("resource_type"), "missing resource type");
  const float amount = effect["amount"].asFloat();
  const auto resource_type = effect["resource_type"].asString();
  return [=](const Actor *a, float dt) -> bool {
    if (a->getPlayerID() == NO_PLAYER) {
      return true;
    }
    if (resource_type == "victory_point") {
      Game::get()->addVPs(a->getTeamID(), dt * amount, a->getID());
    } else if (resource_type == "requisition") {
      Game::get()->addResources(
          a->getPlayerID(),
          ResourceType::REQUISITION,
          dt * amount,
          a->getID());
    } else {
      invariant_violation("Unknown resource type " + resource_type);
    }
    return true;
  };
}

Effect createEffect(const std::string &name) {
  auto effect = getParam("effects." + name);

  invariant(effect.isMember("type"), "missing effects type");
  const std::string type = effect["type"].asString();

  if (type == "aura") {
    return createAuraEffect(effect);
  } else if (type == "resource") {
    return createResourceEffect(effect);
  } else if (type == "test") {
    return createJSTestEffect(effect);
  } else {
    invariant_violation("unknown effect type " + type);
  }
}

};  // rts

