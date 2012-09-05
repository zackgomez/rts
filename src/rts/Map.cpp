#include "rts/Map.h"
#include <vector>
#include "common/ParamReader.h"
#include "rts/Building.h"
#include "rts/MessageHub.h"
#include "rts/Player.h"
#include "rts/Unit.h"

namespace rts {

Map::Map(const std::string &mapName)
  : name_("maps." + mapName),
    size_(vec2Param(name_ + ".size")),
    color_(vec4Param(name_ + ".minimapColor")) {
}

void Map::init(const std::vector<Player *> &players) {
  // TODO(zack) have it read this from a map file
  // we should also create a "base" macro that expands into a headquarters
  // building and starting hero/unit/base defenses unicorns etc

  // TODO(zack): currently these message are from the game to the game.
  // Perhaps we should create a map id

  invariant(players.size() <= intParam(name_ + ".players"),
      "too many players for map");

  for (int i = 0; i < players.size(); i++) {
    const Player *p = players[i];
    id_t pid = p->getPlayerID();
    int ind = pid - STARTING_PID;

    glm::vec3 pos = glm::vec3(
        toVec2(getParam(name_ + ".startingPositions")[ind]), 0);
    glm::vec3 dir = glm::vec3(glm::normalize(
      toVec2(getParam(name_ + ".startingDirections")[ind])), 0);
    glm::vec3 tangent = glm::cross(glm::vec3(0, 0, 1), dir);
    float angle = rad2deg(acosf(glm::dot(glm::vec3(1, 0, 0), dir)));
    float sign =
      glm::dot(glm::cross(glm::vec3(0, 0, 1), glm::vec3(1, 0, 0)), dir);
    if (sign < 0) {
      angle = -angle;
    }

    // Spawn starting building
    Json::Value params;
    params["entity_pid"] = toJson(pid);
    params["entity_pos"] = toJson(pos);
    params["entity_angle"] = angle;
    MessageHub::get()->sendSpawnMessage(
      MAP_ID,
      Building::TYPE,
      "building",
      params);

    // Spawn starting units
    // TODO(zack): get rid of these hardcoded numbers (2.0f, 1.5f)
    pos += dir * 2.0f;
    pos -= tangent * 1.5f;
    for (int i = 0; i < 3; i++) {
      Json::Value params;
      params["entity_pid"] = toJson(pid);
      params["entity_pos"] = toJson(pos);
      params["entity_angle"] = angle;
      MessageHub::get()->sendSpawnMessage(
        MAP_ID,  // from
        Unit::TYPE,  // class
        "melee_unit",  // name
        params);
      pos += tangent * 1.5f;
    }
  }

  Json::Value victoryPoints = getParam(name_ + ".victoryPoints");
  for (int i = 0; i < victoryPoints.size(); i++) {
    glm::vec2 pos = toVec2(victoryPoints[i]);
    Json::Value params;
    params["entity_pid"] = toJson(NO_PLAYER);
    params["entity_pos"] = toJson(pos);
    MessageHub::get()->sendSpawnMessage(
        MAP_ID,
        Building::TYPE,
        "victory_point",
        params);
  }
  Json::Value reqPoints = getParam(name_ + ".reqPoints");
  for (int i = 0; i < reqPoints.size(); i++) {
    glm::vec2 pos = toVec2(reqPoints[i]);
    Json::Value params;
    params["entity_pid"] = toJson(NO_PLAYER);
    params["entity_pos"] = toJson(pos);
    MessageHub::get()->sendSpawnMessage(
        MAP_ID,
        Building::TYPE,
        "req_point",
        params);
  }
}
};  // rts
