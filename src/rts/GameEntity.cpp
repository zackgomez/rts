#include "rts/GameEntity.h"
#include "common/Checksum.h"
#include "common/Collision.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"

namespace rts {

GameEntity::GameEntity(
    id_t id,
    const std::string &name,
    const Json::Value &params,
    bool targetable,
    bool collidable)
  : ModelEntity(id),
    playerID_(NO_PLAYER),
    name_(name),
    targetable_(targetable),
    collidable_(collidable) {
  if (params.isMember("entity_pid")) {
    playerID_ = toID(params["entity_pid"]);
  }
  if (params.isMember("entity_pos")) {
    setPosition(toVec2(params["entity_pos"]));
  }
  if (params.isMember("entity_size")) {
    setSize(toVec2(params["entity_size"]));
  } else {
    setSize(vec2Param("size"));
  }
  if (params.isMember("entity_angle")) {
    setAngle(params["entity_angle"].asFloat());
  }
  if (hasParam("height")) {
    setHeight(param("height"));
  }

  // Make sure we did it right
  assertPid(playerID_);
}

GameEntity::~GameEntity() {
}

id_t GameEntity::getTeamID() const {
  // No player, no team
  if (playerID_ == NO_PLAYER) {
    return NO_TEAM;
  }
  // A bit inefficient but OK for now
  return Game::get()->getPlayer(playerID_)->getTeamID();
}

void GameEntity::update(float dt) {
  setTurnSpeed(0.f);
  setSpeed(0.f);
}

void GameEntity::handleMessage(const Message &msg) {
  if (msg["type"] == MessageTypes::COLLISION) {
    invariant(collidable_, "Got collision for noncollidable object!");
  } else {
    LOG(INFO) << "Entity of type " << getType()
      << " received unknown message type: " << msg["type"] << '\n';
  }
}

void GameEntity::checksum(Checksum &chksum) const {
  id_t id = getID();
  chksum
    .process(id)
    .process(playerID_)
    .process(name_)
    .process(targetable_)
    .process(getPosition())
    .process(getAngle())
    .process(getSize())
    .process(getHeight())
    .process(getSpeed())
    .process(getTurnSpeed());
}

float GameEntity::param(const std::string &p) const {
  return fltParam(name_ + "." + p);
}

std::string GameEntity::strParam(const std::string &p) const {
  return ::strParam(name_ + "." + p);
}

glm::vec2 GameEntity::vec2Param(const std::string &p) const {
  return ::vec2Param(name_ + "." + p);
}

bool GameEntity::hasParam(const std::string &p) const {
  return ::hasParam(name_ + "." + p);
}
};  // rts
