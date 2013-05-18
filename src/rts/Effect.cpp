#include "rts/Effect.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
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
      MessageHub::get()->sendVPMessage(
          a->getID(),
          a->getTeamID(),
          dt * amount);
    } else if (resource_type == "requisition") {
      MessageHub::get()->sendResourceMessage(
          a->getID(),
          a->getPlayerID(),
          ResourceTypes::REQUISITION,
          dt * amount);
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
  } else {
    invariant_violation("unknown effect type " + type);
  }
}

};  // rts
